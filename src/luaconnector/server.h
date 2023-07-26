#pragma once

#include "common.h"
#include "message.h"
#include "connection.h"
#include "../core/tsbuffer.h"

using json = nlohmann::json;

namespace LuaConnector {

namespace Net {

enum MESSAGE_TYPES {
    READ_BYTE = 0x00,
    READ_USHORT = 0x01,
    READ_UINT = 0x02,
    READ_BLOCK = 0x0F,
    WRITE_BYTE = 0x10,
    WRITE_USHORT = 0x11,
    WRITE_UINT = 0x12,
    WRITE_BLOCK = 0x1F,
    ATOMIC_BIT_FLIP = 0x20,
    ATOMIC_BIT_UNFLIP = 0x21,
    MEMORY_FREEZE_UNSIGNED = 0x30,
    MEMORY_UNFREEZE = 0x3F,
    LOAD_ROM = 0xE0,
    UNLOAD_ROM = 0xE1,
    GET_ROM_PATH = 0xE2,
    GET_EMULATOR_CORE_ID = 0xE3,
    CORESTATE_LOAD = 0xE4,
    CORESTATE_SAVE = 0xE5,
    CORESTATE_DELETE = 0xE6,
    CORESTATE_RELOAD = 0xE7,
    PRINT_MESSAGE = 0xF0,
    DO_NOTHING = 0xFF
};

/// <summary>
/// asio server based on olc c++ networking tutorial
/// </summary>
class Server {
public:
    Server(tsbuffer<uint8_t>&);
    ~Server();

    bool Start();
    void Stop();

    bool IsListening();
    bool ClientIsConnected();

    // Main update loop
    // Returns true if we changed the buffer.
    bool Update();

    // Client commands
    //

    // Read data into the buffer.
    // Returns true if buffer was changed.
    bool ReadByteBuffered(uint32_t address);

    // Read data into the buffer.
    // Returns true if buffer was changed.
    bool ReadBlockBuffered(uint32_t address, uint32_t length);

    // Read data without updating the buffer.
    // Returns the requested bytes.
    uint8_t ReadU8Live(uint32_t address);

    // Read data without updating the buffer.
    // Returns the requested bytes.
    uint16_t ReadU16Live(uint32_t address);

    // Do nothing, useful for keepalive
    void do_nothing();

    // Print a message to the screen.
    void print_message(const std::string&);

private:

    // Starts an async accept task.
    // It's safe to use blocking calls with this, I think.
    void waitForClientConnectionAsync();

    // Send a json payload to the client.
    // This method blocks until the operation completes.
    // Returns the json response.
    json sendJsonMessage(const json&);

    // Send a message to the client.
    // This method blocks until the operation completes.
    // Returns the client's response.
    Message messageClient(const Message&);

    // Anything server should prepare on client connect
    void onClientConnect();
    // Anything server should clean up on client disconnect
    void onClientDisconnect();

    // Returns true if data was changed.
    bool updateBuffer(const json&);

    bool _isListening = false;

    uint16_t _CONNECTORLIB_PORT = 43884;

    tsbuffer<uint8_t>& _data;

    // Async message queue
    //tsqueue<Message> _qMessagesIn;

    // order of declaration is important - it is also the order of initialization
    asio::io_context _context;
    std::thread _threadContext;

    asio::ip::tcp::acceptor _acceptor;

    uint32_t _idCounter = 10000;

    std::unique_ptr<Connection> _connection;

    std::chrono::time_point<std::chrono::system_clock> _lastKeepalive;
};

}

}
