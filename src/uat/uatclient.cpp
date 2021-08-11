#include "uatclient.h"
#include <nlohmann/json.hpp>
using json = nlohmann::json;
namespace chrono = std::chrono;


#define MINIMAL_LOGGING


UATClient::UATClient()
{
    valijson::SchemaParser parser;
    parser.populateSchema(JsonSchemaAdapter(_packetSchemaJson), _packetSchema);
    parser.populateSchema(JsonSchemaAdapter(_infoSchemaJson), _infoSchema);
    parser.populateSchema(JsonSchemaAdapter(_varSchemaJson), _varSchema);
    parser.populateSchema(JsonSchemaAdapter(_errorReplySchemaJson), _errorReplySchema);

#ifdef MINIMAL_LOGGING
    _client.clear_access_channels(websocketpp::log::alevel::all);
    _client.set_access_channels(websocketpp::log::alevel::none | websocketpp::log::alevel::app);
#else
    _client.set_access_channels(websocketpp::log::alevel::all);
    _client.clear_access_channels(websocketpp::log::alevel::frame_payload | websocketpp::log::alevel::frame_header);
#endif
    _client.clear_error_channels(websocketpp::log::elevel::all);
    _client.set_error_channels(websocketpp::log::elevel::warn|websocketpp::log::elevel::rerror|websocketpp::log::elevel::fatal);

    _client.init_asio(&_service);

    _client.set_message_handler([this] (websocketpp::connection_hdl hdl, WSClient::message_ptr msg) {
        this->on_message(hdl,msg);
    });
    _client.set_open_handler([this] (websocketpp::connection_hdl hdl) {
        this->on_open(hdl);
    });
    _client.set_close_handler([this] (websocketpp::connection_hdl hdl) {
        this->on_close(hdl);
    });
    _client.set_fail_handler([this] (websocketpp::connection_hdl hdl) {
        this->on_fail(hdl);
    });
}

UATClient::~UATClient()
{
    disconnect(websocketpp::close::status::going_away);
    auto start = chrono::steady_clock::now();
    do {
        _service.poll();
        if (_state == State::DISCONNECTED) break;
    } while (chrono::duration_cast<chrono::milliseconds> (chrono::steady_clock::now() - start).count() < 10);
}

bool UATClient::connect(const std::vector<std::string>& uris)
{
    if (_state != State::DISCONNECTED) return true;
    auto now = chrono::steady_clock::now();

    if (uris != _uris) {
        _uris = uris;
        _nextUri = 0;
    } else if (_nextUri == 0) {
        // limit connection rate
        auto d = chrono::duration_cast<chrono::milliseconds> (now - _connectTimer).count();
        if (d<5000) return false;
    }
    
    _reconnectTimer = now;
    _connectTimer = now;
    for (size_t i = 0; i < _uris.size(); i++) {
        const auto& uri = uris[_nextUri];
        _nextUri = (_nextUri + 1) % _uris.size();
        log("connecting to " + uri);
        _client.reset(); // _service.poll() is a no-op after close otherwise
        websocketpp::lib::error_code ec;
        _conn = _client.get_connection(uri, ec);
        if (ec) {
            log("Could not create connection: " + ec.message());
            continue;
        }
        if (_client.connect(_conn)) {
            _state = State::CONNECTING;
            return true;
        }
    }
    return false;
}

bool UATClient::disconnect(websocketpp::close::status::value status, const std::string& message)
{
    std::string msg = message;
    _service.post([this,status,msg]() {
        log("disconnect (" + std::to_string((int)_state) + ")");
        if (_conn) {
            _state = State::DISCONNECTING;
            _conn->close(status, msg);
        } else {
            _state = State::DISCONNECTED;
        }
    });
    _service.poll();

    return true;
}

void UATClient::log(const std::string& msg)
{
    _client.get_alog().write(websocketpp::log::alevel::app, "UAT: " + msg);
}

void UATClient::on_message(websocketpp::connection_hdl hdl, WSClient::message_ptr msg)
{
#ifndef MINIMAL_LOGGING
    log("message");
#endif
    std::vector<Var> vars;
    try {
        json packet = json::parse(msg->get_payload());
        valijson::Validator validator;
        JsonSchemaAdapter packetAdapter(packet);
        if (!validator.validate(_packetSchema, packetAdapter, nullptr)) {
            throw std::runtime_error("Packet validation failed");
        }
        for (auto& command: packet) {
            std::string cmd = command["cmd"];
            JsonSchemaAdapter commandAdapter(command);
            if (_state == State::SOCKET_CONNECTED && cmd != "Info") {
                throw std::runtime_error("First command has to be Info");
            }
            if (cmd == "Info") {
                if (!validator.validate(_infoSchema, commandAdapter, nullptr)) {
                    throw std::runtime_error("Invalid Info command");
                }
                _state = State::GAME_CONNECTED;
                Info info(command);
                log("Connected to " + info.toShortString());
                if (_onInfo) _onInfo(info);
                
            }
            if (cmd == "Var") {
                if (!validator.validate(_varSchema, commandAdapter, nullptr)) {
                    throw std::runtime_error("Invalid Var command");
                }
                vars.push_back(command);
            }
            if (cmd == "ErrorReply") {
                if (!validator.validate(_errorReplySchema, commandAdapter, nullptr)) {
                    throw std::runtime_error("Invalid ErrorReply command");
                }
                log(command.dump());
            }
        }
    } catch (std::exception& ex) {
        log(ex.what());
        _state = State::DISCONNECTING;
        _client.get_con_from_hdl(hdl)->close(websocketpp::close::status::invalid_subprotocol_data, ex.what());
    }
    if (!vars.empty() && _onVar) _onVar(vars);
}

void UATClient::on_open(websocketpp::connection_hdl hdl)
{
    log("open");
    if (_state == State::CONNECTING) {
        _state = State::SOCKET_CONNECTED;
    }
}

void UATClient::on_close(websocketpp::connection_hdl hdl)
{
    log("close");
    _state = State::DISCONNECTED;
    _conn = nullptr;
}

void UATClient::on_fail(websocketpp::connection_hdl hdl)
{
#ifndef MINIMAL_LOGGING
    log("connect failed");
#endif
    _state = State::DISCONNECTED;
    _conn = nullptr;
}

bool UATClient::poll()
{
    auto res = _service.poll();
    if (_state > State::DISCONNECTED && _state < State::GAME_CONNECTED) {
        auto now = chrono::steady_clock::now();
        auto d = chrono::duration_cast<chrono::milliseconds> (now - _connectTimer).count();
        if (d > 5000) {
            log("Info timed out after " + std::to_string(d));
            _connectTimer = now;
            disconnect();
        }
    }
    return res > 0;
}

void UATClient::sync(const std::string& slot)
{
    if (_conn && _state != State::DISCONNECTED && _state != State::DISCONNECTED) {
        json sync = { {
            { "cmd", "Sync" },
            { "slot", slot },
        }, };
        _conn->send(sync.dump(), websocketpp::frame::opcode::text);
    }
}
