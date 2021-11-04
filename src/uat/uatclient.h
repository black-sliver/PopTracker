#ifndef _UATCLIENT_H_INCLUDED
#define _UATCLIENT_H_INCLUDED


#define ASIO_STANDALONE
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>
#include <nlohmann/json.hpp>
#include <valijson/adapters/nlohmann_json_adapter.hpp>
#include <valijson/schema.hpp>
#include <valijson/schema_parser.hpp>
#include <valijson/validator.hpp>
#include <vector>
#include <string>
#include <chrono>
#include <functional>


class UATClient {
protected:
    typedef websocketpp::client<websocketpp::config::asio_client> WSClient;
    typedef nlohmann::json json;
    typedef valijson::adapters::NlohmannJsonAdapter JsonSchemaAdapter;

public:
    static constexpr auto DEFAULT_URI = "ws://localhost:65399";
    static constexpr auto FALLBACK_URI = "ws://localhost:44444";
    
    enum class State {
        DISCONNECTED = 0,
        CONNECTING,
        SOCKET_CONNECTED,
        GAME_CONNECTED,
        DISCONNECTING,
    };
    
    struct Var {
        std::string slot;
        std::string name;
        json value;
        Var(){}
        Var(const std::string& sl, const std::string& na, const json& val)
                : slot(sl), name(na), value(val) {}
        Var(const json& command) {
            auto slotIt = command.find("slot");
            if (slotIt != command.end() && slotIt->is_string()) slot = *slotIt;
            name = command["name"];
            value = command["value"];
        }
    };
    struct Info {
        std::string name;
        std::string version;
        std::vector<std::string> slots;
        Info(){}
        Info(const json& command) {
            auto it = command.find("name");
            if (it != command.end() && it->is_string()) name = *it;
            it = command.find("version");
            if (it != command.end() && it->is_string()) version = *it;
            it = command.find("slots");
            if (it != command.end() && it->is_array()) {
                for (const auto& slot: *it) slots.push_back(slot);
            }
        }
        std::string toShortString() const {
            std::string s = name.empty() ? "<unknown>" : sanitize(name);
            return version.empty() ? s : (s + " " + sanitize(version));
        }
    };

    UATClient();
    virtual ~UATClient();

    bool connect(const std::vector<std::string>& uris = {DEFAULT_URI, FALLBACK_URI});
    bool disconnect(websocketpp::close::status::value status = websocketpp::close::status::normal,
                    const std::string& message = "", int wait=0);

    bool poll();

    State getState() const { return _state; }
    void setInfoHandler(std::function<void(const Info&)> fn) { _onInfo = fn; }
    void setVarHandler(std::function<void(std::vector<Var>&)> fn) { _onVar = fn; }
    void sync(const std::string& slot="");

protected:
    asio::io_service _service;
    WSClient _client;
    WSClient::connection_ptr _conn;
    bool _gotInfo = false;
    State _state = State::DISCONNECTED;
    std::chrono::steady_clock::time_point _connectTimer; // for connect timeout (no Info)
    std::chrono::steady_clock::time_point _reconnectTimer; // to limit reconnect rate
    std::vector<std::string> _uris;
    size_t _nextUri = 0;
    
    const json _packetSchemaJson = R"({
        "type": "array",
        "items": {
            "type": "object",
            "properties": {
                "cmd": { "type": "string" }
            },
            "required": [ "cmd" ]
        }
    })"_json;
    const json _infoSchemaJson = R"({
        "type": "object",
        "properties": {
            "protocol": { "type": "integer" },
            "name": { "type": "string" },
            "version": { "type": "string" },
            "features": {
                "type": "array",
                "items": { "type": "string" }
            },
            "slots" : {
                "type": "array",
                "items": { "type": "string" }
            }
        },
        "required": [ "protocol" ]
    })"_json;
    const json _varSchemaJson = R"({
    "type": "object",
        "properties": {
            "name": { "type": "string" },
            "slot": { "type": "string" },
            "value": {}
        },
        "required": [ "name", "value" ]
    })"_json;
    const json _errorReplySchemaJson = R"({
        "type": "object",
        "properties": {
            "name": { "type": "string" },
            "reason": { "type": "string" },
            "argument": { "type": "string" },
            "description": { "type": "string" }
        },
        "required": [ "name", "reason" ]
    })"_json;
    valijson::Schema _packetSchema;
    valijson::Schema _infoSchema;
    valijson::Schema _varSchema;
    valijson::Schema _errorReplySchema;

    std::function<void(const Info&)> _onInfo;
    std::function<void(std::vector<Var>&)> _onVar;
    
    void log(const std::string& msg);
    void on_message(websocketpp::connection_hdl hdl, WSClient::message_ptr msg);
    void on_open(websocketpp::connection_hdl hdl);
    void on_close(websocketpp::connection_hdl hdl);
    void on_fail(websocketpp::connection_hdl hdl);

    static std::string sanitize(const std::string& s) {
        std::string res = s;
        for (auto& c: res) if (c < 0x20) c = '?';
        return res;
    }
};
    
#endif // _UATCLIENT_H_INCLUDED
