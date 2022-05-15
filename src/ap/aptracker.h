#ifndef _AP_APTRACKER_H
#define _AP_APTRACKER_H

// APTracker wraps APClient to provide a simpler interface to core/AutoTracker
#include <apclientpp/apclient.hpp>
#include <algorithm>
#include <string>
#include <map>
#include <list>
#include "../core/signal.h"
#include "../core/fileutil.h"
#include <stdlib.h>
#include <utime.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <random>
#include <stdint.h>


class APTracker final {
    typedef nlohmann::json json;
public:
    APTracker(const std::string& appname, const std::string& game="")
            : _appname(appname), _game(game)
    {
        // AP uses an UUID to detect reconnects (replace old connection). If
        // stored UUID is older than 60min, the connection should already be
        // dropped, so we can generate a new one.
        bool newUuid = false;
        auto uuidFilename = getConfigPath(_appname, UUID_FILENAME);
        struct stat st;
        if (stat(uuidFilename.c_str(), &st) == 0) {
            time_t elapsed = time(nullptr) - st.st_mtime;
            newUuid = elapsed > 3600;
        }
        if (!newUuid) newUuid = !readFile(uuidFilename, _uuid);
        if (!newUuid) newUuid = _uuid.empty();
        if (newUuid) {
            // generate a new uuid thread-safe
            _uuid.clear();
            const char uuid_chars[] = "0123456789abcdef";
            std::random_device dev;
            std::mt19937 rng(dev());
            std::uniform_int_distribution<std::mt19937::result_type> dist(0,strlen(uuid_chars)-1);
            for (int n=0; n<16; n++) {
                _uuid.append(1, uuid_chars[dist(rng)]);
            }
            writeFile(uuidFilename, _uuid);
        } else {
            // update mtime of uuid file
            utime(uuidFilename.c_str(), nullptr);
        }
        std::string s;
        if (readFile(getConfigPath(_appname, DATAPACKAGE_FILENAME), s)) {
            try {
                _datapackage = json::parse(s);
            } catch (...) { }
        }
    }

    virtual ~APTracker()
    {
        disconnect(true);
    }

    bool connect(const std::string& uri, const std::string& slot, const std::string& pw)
    {
        disconnect(true);

        if (uri.empty()) {
            fprintf(stderr, "APTracker: uri can't be empty!\n");
            onError.emit(this, "Invalid server");
            return false;
        }
        if (slot.empty()) {
            fprintf(stderr, "APTracker: slot can't be empty!\n");
            onError.emit(this, "Invalid slot name");
            return false;
        }

        _ap = new APClient(_uuid, _game, uri);
        _ap->set_data_package(_datapackage);
        _ap->set_socket_connected_handler([this, slot, pw]() {
            auto lock = EventLock(_event);
            std::list<std::string> tags = {"Tracker"};
            if (_game.empty()) tags.push_back("IgnoreGame");
            return _ap->ConnectSlot(slot, pw, 0b111, tags);
        });
        _ap->set_slot_refused_handler([this](const std::list<std::string>& errs) {
            auto lock = EventLock(_event);
            auto contains = [](const std::list<std::string>& list, const std::string& element) {
                return std::find(list.begin(), list.end(), element) != list.end();
            };
            if (errs.empty()) {
                onError.emit(this, "Connection refused, no reason provided.");
                goto clean_up;
            }
            for (const auto& pair : _errorMessages) {
                if (contains(errs, pair.first)) {
                    onError.emit(this, pair.second);
                    goto clean_up;
                }
            }
            onError.emit(this, "Connection refused: " + errs.front());
        clean_up:
            disconnect();
        });
        _ap->set_slot_connected_handler([this](const json& slotData) {
            auto lock = EventLock(_event);
            onStateChanged.emit(this, _ap->get_state());
            printf("TODO: clean up state\n");
            _itemIndex = 0;
            onClear.emit(this, slotData);

            // update mtime of uuid file
            touchUUID();
        });
        _ap->set_slot_disconnected_handler([this]() {
            auto lock = EventLock(_event);
            onStateChanged.emit(this, _ap->get_state());
        });
        _ap->set_data_package_changed_handler([this](const json& datapackage) {
            _datapackage = datapackage;
            writeFile(getConfigPath(_appname, DATAPACKAGE_FILENAME), _datapackage.dump());
        });
        _ap->set_bounced_handler([this](const json& packet) {
            auto lock = EventLock(_event);
            onBounced.emit(this, packet);
        });
        _ap->set_items_received_handler([this](const std::list<APClient::NetworkItem>& items) {
            auto lock = EventLock(_event);
            for (const auto& item: items) {
                if (item.index != _itemIndex) continue;
                // TODO: Sync if item.index > _itemIndex
                onItem.emit(this, item.index, item.item, _ap->get_item_name(item.item), item.player);
                _itemIndex++;
            }
        });
        _ap->set_location_checked_handler([this](const std::list<int64_t>& locations) {
            auto lock = EventLock(_event);
            for (int location: locations) {
                onLocationChecked.emit(this, location, _ap->get_location_name(location));
            }
        });
        _ap->set_location_info_handler([this](const std::list<APClient::NetworkItem>& scouts) {
            auto lock = EventLock(_event);
            for (const auto& scout: scouts) {
                onScout.emit(this, scout.location, _ap->get_location_name(scout.location),
                        scout.item, _ap->get_item_name(scout.item), scout.player);
            }
        });
        return true;
    }

    void disconnect(bool force=false)
    {
        if (_event && !force) {
            // we can not stop the service while a callback is being executed
            _scheduleDisconnect = true;
            return;
        }
        bool wasConnected = _ap && _ap->get_state() != APClient::State::DISCONNECTED;
        if (_ap) delete _ap;
        _ap = nullptr;
        if (wasConnected)
            onStateChanged.emit(this, APClient::State::DISCONNECTED);
        _scheduleDisconnect = false;
    }

    bool poll()
    {
        if (_event) {
            fprintf(stderr, "APTracker: polling from a signal is unsupported\n");
            return false;
        }
        if (_scheduleDisconnect) disconnect();
        if (_ap) _ap->poll();
        return false;
    }

    void touchUUID()
    {
        // update mtime of the uuid file so we reuse the uuid on next connect
        utime(getConfigPath(_appname, UUID_FILENAME).c_str(), nullptr);
    }

    int getPlayerNumber() const
    {
        return _ap ? _ap->get_player_number() : -1;
    }

    Signal<const std::string&> onError;
    Signal<APClient::State> onStateChanged;
    Signal<const json&> onClear; // called when state has to be cleared, gives new slot_data
    Signal<int, int, const std::string&, int> onItem; // index, item, item_name, player
    Signal<int64_t, const std::string&, int64_t, const std::string&, int> onScout; // location, location_name, item, item_name, target player
    Signal<int64_t, const std::string&> onLocationChecked; // location, location_name
    Signal<const json&> onBounced; // packet

private:
    APClient* _ap = nullptr;
    std::string _appname;
    std::string _game;
    std::string _uuid;
    json _datapackage;
    int _itemIndex = 0;
    std::set<int> _checkedLocations;
    std::set<int> _uncheckedLocations;
    int _event = 0;
    bool _scheduleDisconnect = false;

    static const std::map<std::string, std::string> _errorMessages;

    constexpr static char DATAPACKAGE_FILENAME[] = "ap-datapackage.json";
    constexpr static char UUID_FILENAME[] = "ap-uuid.txt";

    struct EventLock final
    {
        EventLock(int& event) : _event(event) { _event++; }
        ~EventLock() { _event--; }
    private:
        int& _event;
    };
};

#endif // _AP_APTRACKER_H