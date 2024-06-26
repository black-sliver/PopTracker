#include "usb2snes.h"
#include <cstdio>
#include <thread>
#include <mutex>
#include <chrono>
#include <algorithm>
#include <ctime>
#include <vector>
#include <nlohmann/json.hpp>
using json = nlohmann::json;


// TODO: clean this up, maybe split the ws part off?

#define MINIMAL_LOGGING
//#define VERBOSE
//#define TIME_READ
//#ifndef DETACH_THREAD_ON_EXIT // avoid blocking on delete(). not yet implemented


bool USB2SNES::wsConnected()
{
    std::lock_guard<std::mutex> statelock(statemutex);
    return ws_connected;
}
bool USB2SNES::snesConnected()
{
    std::lock_guard<std::mutex> statelock(statemutex);
    return snes_connected;
}

static const json jSCAN = {
    { "Opcode", "DeviceList" },
    { "Space", "SNES" }
};
static const json jGETVERSION = {
    { "Opcode", "AppVersion" },
    { "Space", "SNES" }
};
static const json jINFO = {
    { "Opcode", "Info" },
    { "Space", "SNES" }
};

USB2SNES::USB2SNES(const std::string& name)
{
    // set name sent to (Q)usb2snes
    appname = name;
    // append a random sequence to it
    std::srand(std::time(nullptr));
    const char idchars[] = "0123456789abcdef";
    for (int i=0; i<4; i++) appid += idchars[std::rand()%strlen(idchars)];
    
#ifdef MINIMAL_LOGGING
    client.clear_access_channels(websocketpp::log::alevel::all);
    client.set_access_channels(websocketpp::log::alevel::none | websocketpp::log::alevel::app);
#else
    client.set_access_channels(websocketpp::log::alevel::all);
    client.clear_access_channels(websocketpp::log::alevel::frame_payload | websocketpp::log::alevel::frame_header);
#endif
    client.clear_error_channels(websocketpp::log::elevel::all);
    client.set_error_channels(websocketpp::log::elevel::warn|websocketpp::log::elevel::rerror|websocketpp::log::elevel::fatal);
    
    client.init_asio();
    
    client.set_message_handler([this] (websocketpp::connection_hdl hdl, WSClient::message_ptr msg)
    {
        std::unique_lock<std::mutex> lock(workmutex);
        {
            std::lock_guard<std::mutex> wslock(wsmutex);
            if (!ws_open) return; // shutting down
        }
        
        switch (last_op) {
            case Op::GET_VERSION:
            {
                json res = json::parse(msg->get_payload());
                auto results = res.find("Results");
                if (results != res.end() && results->size()>0) {
                    // FIXME: sanitize usb2snes_version and qusb2snes_version (they may be printed to terminal)
                    usb2snes_version = results->at(0).get<std::string>();
                    if (is_qusb2snes_uri && usb2snes_version.rfind("QUsb2Snes-",0)==0)
                        qusb2snes_version = usb2snes_version.substr(10);
                    else
                        qusb2snes_version.clear();
                }
                if (!qusb2snes_version.empty())
                    printf("QUsb2Snes version: %s\n", qusb2snes_version.to_string().c_str());
                else
                    printf("Usb2Snes version: %s\n", usb2snes_version.c_str());
                optimum_read_block_size = (qusb2snes_version == Version{0,7,19}) ? 128 : 512; // work around performance regression
                break;
            }
            case Op::SCAN:
            {
                json res = json::parse(msg->get_payload());
                auto results = res.find("Results");
                // HANDLE RESULT
                if (results != res.end()) {
                    if (results->size() > 0) {
                        printf("Got %u scan results: %s (last %u)\n", (unsigned)results->size(), results->dump().c_str(), (unsigned)last_dev);
                        if (last_dev>=results->size()) last_dev=0;
                        last_dev_name = results->at(last_dev);
                        printf("Connecting to %s\n", last_dev_name.c_str());
                        json jCONN = {
                            {"Opcode", "Attach"},
                            {"Space", "SNES"},
                            {"Operands", {last_dev_name}}
                        };
                        last_op = Op::CONNECT;
                        client.send(hdl,jCONN.dump(),websocketpp::frame::opcode::text);
                        client.send(hdl,jINFO.dump(),websocketpp::frame::opcode::text);
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                        return;
                    }
                }
                break;
            }
            case Op::CONNECT:
            {
                features.clear();
                if (strncmp(msg->get_payload().c_str(),"USBA", 4)==0) {
                    // if we get this, there was probably crap in the receive buffer of qusb2snes.
                    printf("Received invalid response. Ignoring.\n");
                    return; // Actual reply should follow
                }
#ifdef VERBOSE
                printf("Connect result: [%u] %s\n", msg->get_payload().size(),
                                                    msg->get_payload().c_str());
#endif
                try {
                    std::lock_guard<std::mutex> statelock(statemutex);
                    json res = json::parse(msg->get_payload());
                    auto results = res.find("Results");
                    if (results != res.end() && results->size()>0) { // check more
                        // FIXME: sanitize backend and backend_version (they may be printed to terminal)
                        snes_connected = true;
                        state_changed = true;
                        if (results->size()>1)
                            backend = results->at(1).get<std::string>();
                        else
                            backend = "SD2SNES";
                        backend_version = Version(results->at(0).get<std::string>());
                        for (auto& res: *results) {
                            if (res.is_string()) {
                                std::string s = res.get<std::string>();
                                if (s.rfind("FEAT_",0)==0 || s.rfind("NO_",0)==0) {
                                    features[s] = true;
                                }
                            }
                        }
                        printf("Connected to %s %s\n", backend.c_str(), backend_version.to_string().c_str());
                        if (backend != "NWAccess") // qusb has the better name for emunw in scan result
                            last_dev_name = backend;
                        last_dev_name = last_dev_name.substr(0, last_dev_name.find(" - ")); // cut away extra from qusb
                        read_holes_are_free = (backend == "SD2SNES") ? 512 : 128; // max hole size is a balance between wss delay and actual read cost, this is for qusb2snes 0.7.19
                        if (backend != "SD2SNES") optimum_read_block_size = 512; // clear any limits set previously if device is "virtual"
                        printf("Optimum read set to %u/%u\n", (unsigned)optimum_read_block_size, (unsigned)read_holes_are_free);
                    } else {
                        last_dev++; // try next device
                    }
                } catch (...) {
                    last_dev++; // try next device
                }
                break;
            }
            case Op::PING:
            {
                // ignore message
                break;
            }
            case Op::READ:
            {
                std::lock_guard<std::mutex> datalock(datamutex);
                #ifndef MINIMAL_LOGGING
                printf("Read result: @$%06x=<%u>0x%02x...\n", (unsigned)last_addr, (unsigned)last_len, (uint8_t)msg->get_payload()[0]);
                #endif
                rxbuf += msg->get_payload();
                unsigned read_len = rxbuf.size();
#ifdef TIME_READ
                unsigned long long t = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
                if (read_len < last_len) {
                    printf("[%04u] Partial read: %u/%u\n", (unsigned)(t%10000), (unsigned)msg->get_payload().size(), last_len);
                } else {
                    printf("[%04u] Completed read: %u/%u\n", (unsigned)(t%10000), read_len, last_len);
                }
#endif
                if (read_len < last_len) {
                    // TODO: we probably should add some sort of receive timeout
                    return; // await more data;
                } else if (read_len != last_len) {
                    printf("Read $%06x expected %u bytes but got %u bytes answer\n", (unsigned)last_addr, last_len, (unsigned)(rxbuf.size()+msg->get_payload().size()));
                }
                if (last_len<read_len) read_len = last_len;
                for (unsigned i=0; i<read_len; i++) {
                    if (data[last_addr+i] != (uint8_t)rxbuf[i])
                        data_changed = true;
                    data[last_addr+i] = (uint8_t)rxbuf[i];
                }
                rxbuf.clear(); 
                break;
            }
            default:
                // FIXME: remove control chars before printing to terminal
                printf("unhandled message: %s\n", msg->get_payload().c_str());
        }
        
        bool tmp_snes_connected;
        {
            std::lock_guard<std::mutex> statelock(statemutex);
            tmp_snes_connected = snes_connected;
        }
        if (!tmp_snes_connected) {
            // limit to 10 times a second
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            // rescan
            last_op = Op::SCAN;
            client.send(hdl,jSCAN.dump(),websocketpp::frame::opcode::text);
        } else {
            std::unique_lock<std::mutex> watchlock(watchmutex);
            const auto it = features.find("NO_ROM_READ");
            bool no_rom_read = (it == features.end()) ? false : it->second;
            auto& actwatchlist = no_rom_read ? no_rom_watchlist : watchlist;
            if (actwatchlist.empty()) {
                watchlock.unlock();
                // limit to 10 times a second
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                // ping
                last_op = Op::PING;
                client.send(hdl,jINFO.dump(),websocketpp::frame::opcode::text);
            } else {
                // read data from watches
                last_op = Op::READ;
                if (last_watch >= actwatchlist.size()) {
                    last_watch = 0;
                    if (update_interval>0) { // limit updates per second
                        unsigned long t = (unsigned long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - last_update).count();
                        {
                            watchlock.unlock();
                            // FIXME: do multiple partial sleeps and break when destruction is requested or update_interval changed
                            // WORK-AROUND: limit sleep time to 1sec
                            unsigned long sleept = (t+1000<update_interval) ? 1000 : (t<update_interval) ? (update_interval-t) : 1;
                            std::this_thread::sleep_for(std::chrono::milliseconds(sleept));
                        }
                        watchlock.lock();
                    }
                }
                if (last_watch == 0) {
                    update_count++;
                    last_update = std::chrono::system_clock::now();
                    std::chrono::duration<double> elapsed = std::chrono::system_clock::now() - last_ups_display;
                    if (elapsed.count() >= 5 && update_count>0) {
                        double ups = (double)update_count / elapsed.count();
                        last_ups_display = std::chrono::system_clock::now();
                        update_count = 0;
                        printf("UPS: %7.3f\n", (float)ups);
                    }
                }
                last_addr = actwatchlist[last_watch];
                last_len  = 1;
                uint32_t tmp = last_addr;
                // read consecutive watches in one go
                while (last_watch+1<actwatchlist.size()) {
                    unsigned next = actwatchlist[last_watch+1];
                    if (next<=tmp) break; // we should never reach this
                    if (next-tmp>read_holes_are_free) break;
                    if (last_len+(next-tmp)>optimum_read_block_size) break;
                    last_len+=(next-tmp);
                    tmp=next;
                    last_watch++;
                }
                char saddr[9]; snprintf(saddr, sizeof(saddr), "%06X", (unsigned)last_addr);
                char slen[9];  snprintf(slen,  sizeof(slen),  "%X",   (unsigned)last_len);
                json jREAD = {
                    {"Opcode", "GetAddress"},
                    {"Space", "SNES"},
                    {"Operands", {saddr,slen}}
                };
                client.send(hdl,jREAD.dump(),websocketpp::frame::opcode::text);
                last_watch++;
            }
        }
    });
    
    client.set_open_handler([this] (websocketpp::connection_hdl hdl)
    {
        std::lock_guard<std::mutex> lock(workmutex);
        {
            std::lock_guard<std::mutex> wslock(wsmutex);
            if (!ws_open) return; // shutting down
        }
        std::string uri = client.get_con_from_hdl(hdl)->get_uri()->str();
        is_qusb2snes_uri = (uri.length()>=7 && uri.compare(uri.length()-7, 7, ":23074/")==0);
        {
            std::lock_guard<std::mutex> statelock(statemutex);
            ws_connected = true;
            snes_connected = false;
            state_changed = true;
        }
        printf("* connection to %s opened *\n", uri.c_str());
                
        static const json jNAME = {
            { "Opcode", "Name" },
            { "Space", "SNES" },
            { "Operands", {appname+" "+appid}}
        };
        client.send(hdl,jNAME.dump(),websocketpp::frame::opcode::text);
        last_op = Op::GET_VERSION;
        client.send(hdl,jGETVERSION.dump(),websocketpp::frame::opcode::text);
    });
    
    client.set_fail_handler([this] (websocketpp::connection_hdl hdl)
    {
        std::lock_guard<std::mutex> lock(workmutex);
#ifdef VERBOSE // will generate an error in websocket's logging facility anyway
        printf("* connection to %s failed *\n",
                client.get_con_from_hdl(hdl)->get_uri()->str().c_str());
#endif
        next_uri++;
        {
            std::lock_guard<std::mutex> statelock(statemutex);
            ws_connected = false;
            snes_connected = false;
            state_changed = true;
        }
        last_op = Op::NONE;
    });
    
    client.set_close_handler([this] (websocketpp::connection_hdl hdl)
    {
        std::lock_guard<std::mutex> lock(workmutex);
        std::string uri = client.get_con_from_hdl(hdl)->get_uri()->str();
        next_uri=0;
        features.clear();
        usb2snes_version.clear();
        qusb2snes_version.clear();
        optimum_read_block_size = 512;
        backend.clear();
        backend_version.clear();
        {
            std::lock_guard<std::mutex> statelock(statemutex);
            if (ws_connected || snes_connected) state_changed = true;
            ws_connected = false;
            snes_connected = false;
        }
        last_op = Op::NONE;
        printf("* connection to %s closed *\n", uri.c_str());
    });
}

USB2SNES::~USB2SNES()
{
    {
        std::lock_guard<std::mutex> watchlock(watchmutex);
        watchlist.clear();
        no_rom_watchlist.clear();
    }
    disconnect();
#ifdef DETACH_THREAD_ON_EXIT
    {
        #error "This needs state owned by worker on heap"
        printf("USB2SNES: detaching worker...\n");
        std::lock_guard<std::mutex> lock(workmutex);
        if (worker.joinable()) worker.detach();
    }
#else
    printf("USB2SNES: joining worker...\n");
    if (worker.joinable()) worker.join();
#endif
}

bool USB2SNES::mayBlockOnExit() const
{
#ifdef DETACH_THREAD_ON_EXIT
    return false;
#else
    return true;
#endif
}

bool USB2SNES::connect(std::vector<std::string> uris)
{
    {
        std::lock_guard<std::mutex> wslock(wsmutex);
        if (ws_open) return false;
        ws_open = true;
    }
    
    if (worker.joinable()) worker.join(); // if previously disconnected
    worker = std::thread([uris,this]() {
        do {
            {
                std::lock_guard<std::mutex> lock(workmutex);
                {
                    std::lock_guard<std::mutex> wslock(wsmutex);
                    if (!ws_open) return true; // shutting down
                }
                if (next_uri>=uris.size()) next_uri=0;
                auto uri = uris[next_uri];
                websocketpp::lib::error_code ec;
                conn = client.get_connection(uri, ec);
                if (ec) {
                    printf("Could not create connection because: %s\n", ec.message().c_str());
                    {
                        std::lock_guard<std::mutex> wslock(wsmutex);
                        ws_open = false;
                        return false;
                    }
                }
                client.connect(conn);
                {
                    std::lock_guard<std::mutex> wslock(wsmutex);
                    ws_open = true;
                    ws_connecting = true;
                }
            }
            client.run();
            {
                std::lock_guard<std::mutex> lock(workmutex);
                ws_connecting = false;
                conn = nullptr;
            }
            client.reset();
            if (next_uri>=uris.size()) { // last in list -> pause
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }
        } while (true);
    });
    return true;
}
bool USB2SNES::disconnect()
{
    printf("USB2SNES: disconnect\n");
    {
        std::lock_guard<std::mutex> lock(workmutex);
        std::lock_guard<std::mutex> wslock(wsmutex);
        if (!ws_open) return true;
        ws_open = false;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    {
        std::lock_guard<std::mutex> lock(workmutex);
        std::string res;
        try {
            if (conn) conn->close(websocketpp::close::status::going_away, res);
        } catch (...) {}
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    return true;
}

USB2SNES::Change USB2SNES::poll()
{
    USB2SNES::Change res = USB2SNES::Change::NONE;
    {
        std::lock_guard<std::mutex> datalock(datamutex);
        if (data_changed)
            res |= Change::DATA;
        data_changed = false;
    }
    {
        std::lock_guard<std::mutex> statelock(statemutex);
        if (state_changed)
            res |= Change::STATE;
        state_changed = false;
    }
    return res;
}
uint32_t USB2SNES::mapaddr(uint32_t addr)
{
    // WRAM
    if ((addr>>16)==0x7e || (addr>>16)==0x7f) return 0xF50000 + (addr&0x1ffff);

    // CART
    switch (mapping) {
        case Mapping::LOROM:
            // FASTROM MIRROR
            if (addr>=0x800000) addr -= 0x800000;
            // SRAM
            if ((addr>>16) >= 0x70 && (addr>>16) <= 0x7f && (addr&0xffff) < 0x8000)
                return 0xe00000 + ((addr>>16)-0x70) * 0x8000 + (addr&0xffff);
            // WRAM in lower banks
            if ((addr&0xffff) < 0x2000)
                return 0xf50000 + (addr&0x1fff);
            // RESERVED / HARDWARE
            if ((addr&0xffff) < 0x8000)
                return 0;
            // ROM
            return ((addr&0x7f0000) >> 1) + (addr&0x7fff); // pretty sure this is correct
            break;

        case Mapping::HIROM:
            // FASTROM MIRROR
            if (addr>=0x800000) addr -= 0x800000;
            // SRAM
            if ((addr>>16) >= 0x20 && (addr>>16) <= 0x3f && (addr&0xffff) >= 0x6000 && (addr&0xffff) <= 0x7fff)
                return 0xe00000 + ((addr>>16)-0x20) * 0x2000 + (addr&0xffff)-0x6000;
            // WRAM in lower banks
            if (addr>=0x000000 && addr<=0x3fffff && (addr&0xffff) < 0x2000)
                return 0xf50000 + (addr&0x1fff);
            // ROM
            return addr & 0x3fffff;
            break;

        case Mapping::EXLOROM:
            // NOTE: this is probably completely wrong, but we need *something*
            // SRAM
            if ((addr>>16) >= 0xf0 && (addr>>16) <= 0xff && (addr&0xffff) < 0x8000)
                return 0xe00000 + ((addr>>16)-0x70) * 0x8000 + (addr&0xffff);
            // WRAM in lower banks
            if ((addr&0xffff) < 0x2000)
                return 0xf50000 + (addr&0x1fff);
            // RESERVED / HARDWARE
            if ((addr&0xffff) < 0x8000)
                return 0;
            // ROM1
            if (addr&0x800000)
                return ((addr&0x7f0000) >> 1) + (addr&0x7fff);
            // ROM2
            return 0x400000 + ((addr&0x7f0000) >> 1) + (addr&0x7fff);

        case Mapping::EXHIROM:
            // ROM1
            if (addr>=0xc00000 && addr<=0xffffff)
                return addr & 0x3fffff;
            if (addr>=0x800000 && addr<=0xbfffff && (addr&0x8000))
                return addr & 0x3f7fff; // this may be wrong
            // ROM2
            if (addr>=0x400000 && addr<0x7e0000)
                return 0x400000 + (addr & 0x3ffff);
            if (addr>=0x000000 && addr<=0x3fffff && (addr&0x8000))
                return 0x400000 + (addr & 0x3f7fff); // this may be wrong
            // SRAM
            if (addr>=0xa00000 && addr<=0xbfffff && (addr&0x7fff)>=0x6000)
                return 0xe00000 + ((addr>>16)-0xa0) * 0x2000 + (addr&0xffff)-0x6000;
            // WRAM in lower banks
            if ((addr&0xffff) < 0x2000)
                return 0xf50000 + (addr&0x1fff);
            break;

        case Mapping::SA1:
            // copied from https://github.com/alttpo/snes/blob/main/mapping/sa1rom/mapping.go
            // ROM
            if (addr>=0xc00000)
                return addr & 0x3fffff;
            // MIXED fast
            if (addr>=0x800000) {
                auto offs = addr & 0xffff;
                auto bank = addr >> 16;
                if (offs >= 0x8000)
                    return ((bank - 0x80 + 0x40) << 15) | (offs & 0x7fff);
                if (offs >= 0x6000)
                    // BW-RAM image dynamically selects a single $2000 sized block
                    return 0xe00000 + (offs - 0x6000);
                if (offs < 0x2000)
                    // WRAM
                    return 0xf50000 + offs;
                break; // unmapped or SA-1 registers
            }
            if (addr>=0x500000) {
                break; // unmapped?
            }
            if (addr>=0x440000) {
		// BW-RAM image dynamically selects a single $2000 sized block
		return 0xe00000 + (addr & 0x1FFF);
            }
            if (addr>=0x400000) {
		// BW-RAM area: linearly mapped
		return 0xe00000 + addr - 0x400000;
            }
            // MIXED slow
            if (addr<0x400000) {
                auto offs = addr & 0xffff;
                auto bank = addr >> 16;
                if (offs >= 0x8000)
                    return (bank << 15) + (offs & 0x7fff);
                if (offs >= 0x6000)
                    // BW-RAM image dynamically selects a single $2000 sized block
                    return 0xe00000 + (offs - 0x6000);
                if (offs < 0x2000)
                    // WRAM
                    return 0xf50000 + offs;
                break; // unmapped or SA-1 registers
            }

        case Mapping::UNKNOWN: // old behavior; TODO: auto-detect when unknown
            return addr&0x3ffff; // NOTE: this can not access SRAM
            break;
    }

    // unmapped
    return 0;
}
static bool is_rom(uint32_t usb2snes_addr)
{
    return usb2snes_addr < 0xe00000;
}

void USB2SNES::setMapping(USB2SNES::Mapping value)
{
    // FIXME: all old watches are invalid now
    mapping = value;
}

USB2SNES::Mapping USB2SNES::getMapping() const
{
    return mapping;
}

void USB2SNES::addWatch(uint32_t addr, unsigned len)
{
    uint32_t usb2snes_addr = mapaddr(addr);
    {
        std::lock_guard<std::mutex> watchlock(watchmutex);
        for (size_t i=0; i<len; i++) {
            if (std::find(watchlist.begin(), watchlist.end(), usb2snes_addr+i)==watchlist.end())
                watchlist.push_back(usb2snes_addr+i);
        }
        sort(watchlist.begin(), watchlist.end());
        for (size_t i=0; i<len; i++) {
            if (is_rom(usb2snes_addr+i)) continue;
            if (std::find(no_rom_watchlist.begin(), no_rom_watchlist.end(), usb2snes_addr+i)==no_rom_watchlist.end())
                no_rom_watchlist.push_back(usb2snes_addr+i);
        }
        sort(no_rom_watchlist.begin(), no_rom_watchlist.end());
    }
}
void USB2SNES::removeWatch(uint32_t addr, unsigned len)
{
    addr = mapaddr(addr);
    {
        std::lock_guard<std::mutex> watchlock(watchmutex);
        for (size_t i=0; i<len; i++) {
            watchlist.erase(std::remove(watchlist.begin(), watchlist.end(), addr+i), watchlist.end());
        }
        for (size_t i=0; i<len; i++) {
            no_rom_watchlist.erase(std::remove(no_rom_watchlist.begin(), no_rom_watchlist.end(), addr+i), no_rom_watchlist.end());
        }
    }
}
uint8_t USB2SNES::read(uint32_t addr)
{
    uint32_t usb2snes_addr = mapaddr(addr);
#ifdef USB2SNES_ALLOW_READ_WITHOUT_WATCH
    uint8_t val;
    {
        std::lock_guard<std::mutex> datalock(datamutex);
        val = data[usb2snes_addr];
        for (const auto& w: watchlist)
            if (w >= usb2snes_addr) return val;
    }
    // value is not being watched -> add watch
    addWatch(addr);
    return val;
#else
    {
        std::lock_guard<std::mutex> datalock(datamutex);
        return data[usb2snes_addr];
    }
#endif
}

bool USB2SNES::read(uint32_t addr, unsigned len, void* out)
{
    uint8_t* dst = (uint8_t*)out;
    uint32_t usb2snes_addr = mapaddr(addr);
#ifdef USB2SNES_ALLOW_READ_WITHOUT_WATCH
    bool missing = true;
    {
        std::lock_guard<std::mutex> datalock(datamutex);
        for (size_t i=0; i<len; i++) dst[i] = data[usb2snes_addr+i];
        for (size_t i=0; i<watchlist.size(); i++) {
            if (watchlist[i] == usb2snes_addr) {
                if (watchlist.size()>=i+len && watchlist[i+len-1] == usb2snes_addr+len-1)
                    missing = false;
                break;
            } else if (watchlist[i] > usb2snes_addr) {
                break;
            }
        }
    }
    if (missing) addWatch(addr, len);
#else
    {
        std::lock_guard<std::mutex> datalock(datamutex);
        for (size_t i=0; i<len; i++) dst[i] = data[usb2snes_addr+i];
    }
    return true;
#endif
}

bool USB2SNES::hasFeature(std::string feat)
{
    std::lock_guard<std::mutex> lock(workmutex);
    const auto it = features.find(feat);
    if (it == features.end()) return false;
    return it->second;
}

void USB2SNES::clearCache()
{
    std::lock_guard<std::mutex> lock(datamutex);
    data.clear();
}

std::string USB2SNES::getDeviceName()
{
    std::lock_guard<std::mutex> lock(workmutex);
    return last_dev_name;
}

void USB2SNES::nextDevice()
{
    std::lock_guard<std::mutex> lock(workmutex);
    std::lock_guard<std::mutex> statelock(statemutex);
    last_dev++;
    snes_connected = false;
}
