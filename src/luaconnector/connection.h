#pragma once

#include <nlohmann/json.hpp>
#include <asio.hpp>
#include <stdint.h>
#include "message.h"
#include "tsqueue.h"

namespace LuaConnector {

namespace Net {

class Connection {
    using json = nlohmann::json;

public:
#ifdef LUACONNECTOR_NOASYNC
    Connection(asio::io_context&, asio::ip::tcp::socket);
#else
    Connection(asio::io_context&, asio::ip::tcp::socket, tsqueue<Message>& qIn);
#endif
    ~Connection();

    // Anything we should prepare when newly connected
    void ConnectToClient(uint32_t uid = 0);
    // Anything we should clean up when disconnected
    bool Disconnect();

    bool IsConnected() const;

    uint32_t GetID() const;

#ifdef LUACONNECTOR_NOASYNC

public:
    // Send a message to the client, and return its response.
    // This function will block until the full response is received.
    // Do not use with async alternative in the same application.
    Message Send(const Message&);

protected:
    void ReadHeader(Message&);
    void ReadBody(Message&);
    void WriteHeader(const Message&);
    void WriteBody(const Message&);

    asio::io_context& _context;
    asio::ip::tcp::socket _socket;

#else

public:
    // Queue a message to be sent to the client.
    // This function is non-blocking.
    // Do not use with blocking alternative in the same application.
    void SendAsync(const Message&);

protected:
    void ReadHeaderAsync();
    void ReadBodyAsync();
    void WriteHeaderAsync();
    void WriteBodyAsync();

    void AddToIncomingMessageQueue();

    asio::io_context& _context;
    asio::ip::tcp::socket _socket;

    tsqueue<Message> _qMessagesOut;
    tsqueue<Message>& _qMessagesIn;

#endif

protected:
    Message _msgTemporaryIn;

    uint32_t _uid = 0;
    uint32_t _bid = 0;
};

} // namespace Net

} // namespace LuaConnector
