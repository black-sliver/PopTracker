#include "server.h"
#include "../core/tsbuffer.h"
#include <string>
#include <stdint.h>
#include <nlohmann/json.hpp>
#include <asio.hpp>
#include <thread>
#include <memory>
#include <chrono>
#include "message.h"
#include "connection.h"

#include <websocketpp/base64/base64.hpp>
#include <stdio.h>

using asio::ip::tcp;
using json = nlohmann::json;

namespace LuaConnector {

namespace Net {

Server::Server(tsbuffer<uint8_t>& data) : _data(data), _acceptor(_context, tcp::endpoint(tcp::v4(), _CONNECTORLIB_PORT))
{
}

Server::~Server()
{
    Stop();
}

bool Server::Start()
{
    try {
        // give the context work to do when it starts
        waitForClientConnectionAsync();

        // start the context in its thread
        _threadContext = std::thread([this]()
            {
                _context.run();
            });

        _isListening = true;
    }
    catch (const std::exception& e) {
        printf("LuaConnector: Exception: %s\n", e.what());
        return false;
    }

    printf("LuaConnector: Started\n");
    return true;
}

void Server::Stop()
{
    // request the context to close
    _context.stop();

    // wait for thread to stop
    if (_threadContext.joinable()) _threadContext.join();

    _isListening = false;

    printf("LuaConnector: Stopped\n");
}

bool Server::IsListening()
{
    return _isListening;
}

bool Server::ClientIsConnected()
{
    return _connection && _connection->IsConnected();
}

bool Server::Update()
{
    bool data_changed = false;

#ifndef LUACONNECTOR_NOASYNC
    // loop inbound messages
    while (!_qMessagesIn.empty()) {
        auto msg = _qMessagesIn.pop_front();

        json body = msg.GetJson();

        //printf("LuaConnector: Receiving message: %s\n", body.dump().c_str());

        data_changed |= updateBuffer(body);
    }

#endif
    // send keepalive
    if (_connection && _lastKeepalive + std::chrono::milliseconds(2000) < std::chrono::system_clock::now()) {
        _lastKeepalive = std::chrono::system_clock::now();
        do_nothing();
    }

    return data_changed;
}

#ifdef LUACONNECTOR_NOASYNC
bool Server::ReadByteBuffered(uint32_t address)
{
    json body;

    body["type"] = READ_BYTE;
    body["address"] = address;
    body["domain"] = "RDRAM";

    json reply = sendJsonMessage(body);
    return updateBuffer(reply);
}

bool Server::ReadBlockBuffered(uint32_t address, uint32_t length)
{
    json body;

    body["type"] = READ_BLOCK;
    body["address"] = address;
    body["value"] = length;
    body["domain"] = "RDRAM";

    json reply = sendJsonMessage(body);
    return updateBuffer(reply);
}

uint8_t Server::ReadU8Live(uint32_t address)
{
    json block;

    block["type"] = READ_BYTE;
    block["address"] = address;
    block["domain"] = "RDRAM";

    json reply = sendJsonMessage(block);

    uint8_t result = 0;

    if (!reply.is_null())
        result = reply["value"];

    return result;
}

uint16_t Server::ReadU16Live(uint32_t address)
{
    json block;

    block["type"] = READ_USHORT;
    block["address"] = address;
    block["domain"] = "RDRAM";

    json reply = sendJsonMessage(block);

    uint16_t result = 0;

    if (!reply.is_null())
        result = reply["value"];

    return result;
}

#else

void Server::ReadByteBufferedAsync(uint32_t address)
{
    json body;

    body["type"] = READ_BYTE;
    body["address"] = address;
    body["domain"] = "RDRAM";

    sendJsonMessageAsync(body);
}

void Server::ReadBlockBufferedAsync(uint32_t address, uint32_t length)
{
    json body;

    body["type"] = READ_BLOCK;
    body["address"] = address;
    body["value"] = length;
    body["domain"] = "RDRAM";

    sendJsonMessageAsync(body);
}

#endif

void Server::do_nothing()
{
    json body;

    body["type"] = DO_NOTHING;

    sendJsonMessageAsync(body);
}

void Server::print_message(const std::string& messageText)
{
    json body;

    body["type"] = PRINT_MESSAGE;
    body["message"] = messageText;

    sendJsonMessageAsync(body);
}

// async
void Server::waitForClientConnectionAsync()
{
    _acceptor.async_accept(
        [this](std::error_code ec, tcp::socket socket)
        {
            // Incoming connection request
            if (!ec) {
                printf("LuaConnector: New connection: %s:%d\n", socket.remote_endpoint().address().to_string().c_str(), socket.remote_endpoint().port());

                if (_connection == nullptr || !_connection->IsConnected()) {
                    try {
                        if (_connection) {
                            // disconnect the client
                            onClientDisconnect();
                            _connection.reset();
                        }

                        _connection = std::make_unique<Connection>(_context, std::move(socket), _qMessagesIn);

                        _connection->ConnectToClient(_idCounter++);

                        onClientConnect();
                    }
                    catch (const std::exception& e) {
                        printf("LuaConnector: Exception: %s\n", e.what());
                    }
                }
                else {
                    printf("LuaConnector: Rejected, connection in use\n");
                }
            }
            else {
                printf("LuaConnector: New connection error: %s\n", ec.message().c_str());
            }

            // give the context more work - wait for another connection
            waitForClientConnectionAsync();
        }
    );
}

#ifdef LUACONNECTOR_NOASYNC

json Server::sendJsonMessage(const json& j)
{
    Message m(j);

    Message reply = messageClient(m);

    if (reply.body.size() > 0)
        return reply.GetJson();
    else
        return json();
}

Message Server::messageClient(const Message& msg)
{
    if (_connection && _connection->IsConnected()) {
        return _connection->Send(msg);
    }
    else {
        // disconnect the client
        onClientDisconnect();
        _connection.reset();
        return Message();
    }
}

#else

void Server::sendJsonMessageAsync(const json& j)
{
    Message m(j);

    //printf_s("LuaConnector: Sending message: %s\n", j.dump().c_str());

    messageClientAsync(m);
}

void Server::messageClientAsync(const Message& msg)
{
    if (_connection && _connection->IsConnected()) {
        _connection->SendAsync(msg);
    }
    else {
        // disconnect the client
        onClientDisconnect();
        _connection.reset();
    }
}

#endif

void Server::onClientConnect()
{
    //print_message("Connected to PopTracker"); // say hello
}

void Server::onClientDisconnect()
{

}

bool Server::updateBuffer(const json& response)
{
    if (response.is_null())
        return false;

    if (response["type"] == READ_BLOCK) {
        uint32_t address = response["address"];
        size_t length = response["value"];
        std::string encoded = response["block"];
        std::string buffer = websocketpp::base64_decode(encoded);

        //printf("LuaConnector: Update cache at %x: %s\n", address, buffer.c_str());

        return _data.write(address, length, buffer.c_str());
    }

    return false;
}

} // namespace Net

} // namespace LuaConnector
