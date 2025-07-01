#include "connection.h"
#include <nlohmann/json.hpp>
#include <asio.hpp>
#include <stdint.h>
#include "message.h"
#include "tsqueue.h"

#include <stdio.h>

namespace LuaConnector {

namespace Net {

#ifdef LUACONNECTOR_NOASYNC
Connection::Connection(asio::io_context& context, asio::ip::tcp::socket socket)
    : _context(context), _socket(std::move(socket))
{
}
#else
Connection::Connection(asio::io_context& context, asio::ip::tcp::socket socket, tsqueue<Message>& qIn)
    : _context(context), _socket(std::move(socket)), _qMessagesIn(qIn)
{
}
#endif

Connection::~Connection()
{
    printf("LuaConnector: [%u] Disconnected\n", _uid);
}

void Connection::ConnectToClient(uint32_t uid)
{
    try {
        if (_socket.is_open()) {
            _uid = uid;

            printf("LuaConnector: [%u] Connection established\n", _uid);

#ifndef LUACONNECTOR_NOASYNC
            // kick off async task
            ReadHeaderAsync();
#endif
        }
    }
    catch (const std::exception& e) {
        printf("LuaConnector: [%u] Exception: %s\n", _uid, e.what());
    }
}

bool Connection::Disconnect()
{
    if (IsConnected()) {
        asio::post(_context, [this]() { _socket.close(); });
    }

    return false;
}

bool Connection::IsConnected() const
{
    return _socket.is_open();
}

uint32_t Connection::GetID() const
{
    return _uid;
}

#ifdef LUACONNECTOR_NOASYNC

Message Connection::Send(const Message& msg)
{
    if (_socket.is_open()) {
        //printf("LuaConnector: [%u] Send on main thread\n", _uid);

        try {
            Message reply;

            WriteHeader(msg);
            WriteBody(msg);
            ReadHeader(reply);
            ReadBody(reply);

            return reply;
        }
        catch (const std::exception&) {
            printf("LuaConnector: [%u] Communication failure\n", _uid);
            _socket.close();
            return Message();
        }
    }

    return Message();
}

void Connection::ReadHeader(Message& msg)
{
    asio::read(_socket, asio::buffer(&msg.header, sizeof(MessageHeader)));

    msg.header.size = ntohl(msg.header.size);

    if (msg.header.size > 0) {
        msg.body.resize(msg.header.size);
    }
}

void Connection::ReadBody(Message& msg)
{
    asio::read(_socket, asio::buffer(msg.body.data(), msg.header.size));
}

void Connection::WriteHeader(const Message& msg)
{
    asio::write(_socket, asio::buffer(&msg.header, sizeof(MessageHeader)));
}

void Connection::WriteBody(const Message& msg)
{
    asio::write(_socket, asio::buffer(msg.body.data(), msg.body.size()));
}

#else

void Connection::SendAsync(const Message& msg)
{
    //printf("LuaConnector: [%u] Sending Header: %u Body size: %u\n", _uid, ntohl(msg.header.size), msg.body.size());

    asio::post(_context,
        [this, msg]()
        {
            bool notWritingMessage = _qMessagesOut.push_back(msg);

            if (notWritingMessage) {
                WriteHeaderAsync();
            }
        });
}

void Connection::ReadHeaderAsync()
{
    try {
        asio::async_read(_socket, asio::buffer(&_msgTemporaryIn.header, sizeof(MessageHeader)),
            [this](std::error_code ec, const std::size_t length)
            {
                // TODO: handle length < sizeof(MessageHeader)
                if (!ec && length == sizeof(MessageHeader)) {
                    // use network byte order
                    _msgTemporaryIn.header.size = ntohl(_msgTemporaryIn.header.size);

                    if (_msgTemporaryIn.header.size > 0) {
                        // resize message body to make room
                        _msgTemporaryIn.body.resize(_msgTemporaryIn.header.size);

                        // register next async task
                        ReadBodyAsync();
                    }
                    else {
                        AddToIncomingMessageQueue();
                    }
                }
                else {
                    //printf("LuaConnector: [%u] Read header failed: %s\n", _uid, ec.message().c_str());
                    _socket.close();
                }
            });
    }
    catch (const std::exception& e) {
        printf("LuaConnector: [%u] Register async read failed: %s\n", _uid, e.what());
        _socket.close();
    }
}

void Connection::ReadBodyAsync()
{
    try {
        const size_t dataLength = _msgTemporaryIn.header.size;
        asio::async_read(_socket, asio::buffer(_msgTemporaryIn.body.data(), dataLength),
            [this, dataLength](std::error_code ec, const std::size_t length)
            {
                // TODO: handle length < dataLength
                if (!ec && length == dataLength) {
                    AddToIncomingMessageQueue();
                }
                else {
                    //printf("LuaConnector: [%u] Read body failed: %s\n", _uid, ec.message().c_str());
                    _socket.close();
                }
            });
    }
    catch (const std::exception& e) {
        printf("LuaConnector: [%u] Register async read failed: %s\n", _uid, e.what());
        _socket.close();
    }
}

void Connection::WriteHeaderAsync()
{
    try {
        asio::async_write(_socket, asio::buffer(&_qMessagesOut.front().header, sizeof(MessageHeader)),
            [this](std::error_code ec, std::size_t written)
            {
                // TODO: handle written < sizeof(MessageHeader)
                if (!ec && written == sizeof(MessageHeader)) {
                    if (!_qMessagesOut.front().body.empty()) {
                        WriteBodyAsync();
                    }
                    else {
                        bool isEmpty = _qMessagesOut.remove_front();

                        if (!isEmpty) {
                            WriteHeaderAsync();
                        }
                    }
                }
                else {
                    //printf("LuaConnector: [%u] Write header failed: %s\n", _uid, ec.message().c_str());
                    _socket.close();
                }
            });
    }
    catch (const asio::system_error& e) {
        printf("LuaConnector: [%u] Register async write failed: %s\n", _uid, e.what());
        _socket.close();
    }
}

void Connection::WriteBodyAsync()
{
    try {
        const size_t dataLength = _qMessagesOut.front().body.size();
        asio::async_write(_socket, asio::buffer(_qMessagesOut.front().body.data(), dataLength),
            [this, dataLength](std::error_code ec, const std::size_t written)
            {
                // TODO: handle written < dataLen
                if (!ec && written == dataLength) {
                    bool isNowEmpty = _qMessagesOut.remove_front();

                    if (!isNowEmpty) {
                        WriteHeaderAsync();
                    }
                }
                else {
                    //printf("LuaConnector: [%u] Write body failed: %s\n", _uid, ec.message().c_str());
                    _socket.close();
                }
            });
    }
    catch (const std::exception& e) {
        printf("LuaConnector: [%u] Register async write failed: %s\n", _uid, e.what());
        _socket.close();
    }
}

void Connection::AddToIncomingMessageQueue()
{
    // Add complete message to server's incoming message queue
    _qMessagesIn.push_back(_msgTemporaryIn);

    // register next async task
    ReadHeaderAsync();
}

#endif
} // namespace Net

} // namespace LuaConnector
