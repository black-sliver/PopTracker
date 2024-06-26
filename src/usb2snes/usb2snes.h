#ifndef _USB2SNES_H_INCLUDED
#define _USB2SNES_H_INCLUDED

#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>
#include <thread>
#include <mutex>
#include <chrono>
#include <vector>
#include <string>
#include <type_traits>

class USB2SNES {
    public:
        enum class Mapping {
            UNKNOWN,
            LOROM,
            HIROM,
            EXLOROM,
            EXHIROM,
            SA1,
        };

        enum class Change : unsigned {
            NONE = 0,
            STATE = 1,
            DATA = 2,
        };

        USB2SNES(const std::string& appname);
        ~USB2SNES();
        bool connect(std::vector<std::string> uris = {QUSB2SNES_URI,LEGACY_URI});
        bool disconnect();
        Change poll();
        static constexpr auto QUSB2SNES_URI = "ws://localhost:23074";
        static constexpr auto LEGACY_URI = "ws://localhost:8080";
        bool wsConnected();
        bool snesConnected();
        void setMapping(Mapping mapping);
        Mapping getMapping() const;
        void addWatch(uint32_t addr, unsigned len=1);
        void removeWatch(uint32_t addr, unsigned len=1);
        uint8_t read(uint32_t addr);
        bool read(uint32_t addr, unsigned len, void* out);
        template<typename T>
        T readInt(uint32_t addr);
        
        bool hasFeature(std::string feat);
        void setUpdateInterval(size_t interval) { update_interval = interval; }
        void clearCache();
        std::string getDeviceName();
        void nextDevice();

        bool mayBlockOnExit() const;
        
    protected:
        typedef websocketpp::client<websocketpp::config::asio_client> WSClient;
        struct Version {
            int vmajor;
            int vminor;
            int vrevision;
            std::string extra;
            Version(const std::string& vs) {
                char* next = NULL;
                vmajor = (int)strtol(vs.c_str(), &next, 10);
                if (next && *next) vminor = (int)strtol(next+1, &next, 10);
                if (next && *next) vrevision = (int)strtol(next+1, &next, 10);
                if (next && *next) extra = next+1;
            }
            Version(int ma=0,int mi=0, int rev=0, const std::string& ex="")
                    : vmajor(ma), vminor(mi), vrevision(rev), extra(ex) {}
            void clear() { *this = {}; }
            bool empty() const { 
                return vmajor==0 && vminor==0 && vrevision==0 && extra.empty();
            }
            std::string to_string() const {
                return std::to_string(vmajor) + "." +
                       std::to_string(vminor) + "." + 
                       std::to_string(vrevision) + (extra.empty()?"":"-") +
                       extra;
            }
            int compare(const Version& other) const {
                if (other.vmajor>vmajor) return -1;
                else if (other.vmajor<vmajor) return 1;
                if (other.vminor>vminor) return -1;
                else if (other.vminor<vminor) return 1;
                if (other.vrevision>vrevision) return -1;
                else if (other.vrevision<vrevision) return 1;
                return 0;
            }
            bool operator<(const Version& other) const { return compare(other)<0; }
            bool operator>(const Version& other) const { return compare(other)>0; }
            bool operator>=(const Version& other) const { return !(*this<other); }
            bool operator<=(const Version& other) const { return !(*this>other); }
            bool operator==(const Version& other) const { return compare(other)==0; }
            bool operator!=(const Version& other) const { return !(*this==other); }
        };
        WSClient client;
        WSClient::connection_ptr conn;
        // we have 1 mutex per data access
        // + 1 mutex for checking/requesting socket state (wsmutex)
        // + 1 mutex for the actual work(er/socket) (we can not destroy it while it's busy)
        std::mutex wsmutex; // FIXME: this is getting out of hand
        std::mutex workmutex;
        std::mutex datamutex;
        std::mutex watchmutex;
        std::mutex statemutex;
        
        std::thread worker;
        std::string appname;
        std::string appid;
        bool ws_open = false;
        bool ws_connected = false;
        bool ws_connecting = false;
        bool snes_connected = false;
        enum class Op {
            NONE,
            GET_VERSION,
            SCAN,
            CONNECT,
            READ,
            PING,
        };
        Op last_op = Op::NONE;
        std::string last_dev_name;
        size_t last_dev = 0;
        size_t last_watch = 0;
        uint32_t last_addr = 0;
        unsigned last_len = 0;
        std::string rxbuf;
        std::vector<uint32_t> watchlist;
        std::vector<uint32_t> no_rom_watchlist;
        std::map<uint32_t, uint8_t> data;
        bool data_changed = true;
        bool state_changed = true;
        std::chrono::system_clock::time_point last_update;
        std::chrono::system_clock::time_point last_ups_display;
        unsigned long update_count = 0;
        unsigned long update_interval = 0;
        std::map<std::string,bool> features;
        std::string usb2snes_version;
        Version qusb2snes_version;
        std::string backend;
        Version backend_version;
        bool is_qusb2snes_uri = false;
        size_t next_uri = 0;
        size_t optimum_read_block_size = 512; // NOTE: qusb2snes 0.7.19 on linux read takes 20ms/128B, so "native" 512 has worse performance. TODO: fix this in qusb2snes
        size_t read_holes_are_free = true;//false;
        Mapping mapping = Mapping::UNKNOWN;

        uint32_t mapaddr(uint32_t addr);
};

template<typename T>
T USB2SNES::readInt(uint32_t addr)
{
    T res=0;
    addr = mapaddr(addr);
    {
        std::lock_guard<std::mutex> datalock(datamutex);
        for (size_t n=0; n<sizeof(T); n++) {
            res <<= 8;
            res += data[addr+sizeof(T)-n-1];
        }
    }
    return res;
}

static inline USB2SNES::Change operator|(USB2SNES::Change lhs, USB2SNES::Change rhs)
{
    return static_cast<USB2SNES::Change>(
            static_cast<typename std::underlying_type<USB2SNES::Change>::type>(lhs) |
            static_cast<typename std::underlying_type<USB2SNES::Change>::type>(rhs)
    );
}

static inline USB2SNES::Change& operator|=(USB2SNES::Change& lhs, USB2SNES::Change rhs)
{
    return lhs = lhs | rhs;
}

static inline USB2SNES::Change operator&(USB2SNES::Change lhs, USB2SNES::Change rhs)
{
    return static_cast<USB2SNES::Change>(
            static_cast<typename std::underlying_type<USB2SNES::Change>::type>(lhs) &
            static_cast<typename std::underlying_type<USB2SNES::Change>::type>(rhs)
    );
}

static inline bool operator!(USB2SNES::Change e) {
    return e == static_cast<USB2SNES::Change>(0);
}


#endif // _USB2SNES_H_INCLUDED

