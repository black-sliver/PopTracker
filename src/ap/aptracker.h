#pragma once

// APTracker wraps APClient to provide a simpler interface to core/AutoTracker
#include <apclientpp/apclient.hpp>
#include <algorithm>
#include <string>
#include <map>
#include <list>
#include "../core/signal.h"
#include "../core/fileutil.h"
#include "../core/assets.h"
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
    APTracker(const std::string& appname, const std::string& game="", bool allowSend=false, bool allowScout=false)
            : _appname(appname), _game(game), _allowSend(allowSend), _allowScout(allowScout)
    {
        // AP uses a UUID to detect reconnects (replace old connection).
        // If stored UUID is older than 60min, the connection should already be
        // dropped, so we can generate a new one.
        bool newUuid = false;
        auto uuidFilename = getConfigPath(_appname, UUID_FILENAME);
#ifdef _WIN32
        struct _stat64 st;
        auto now = _time64(nullptr);
        if (_wstat64(uuidFilename.c_str(), &st) == 0)
#else
        struct stat st;
        auto now = time(nullptr);
        if (stat(uuidFilename.c_str(), &st) == 0)
#endif
        {
            static_assert(sizeof(st.st_mtime) == 8 && sizeof(now) == 8);
            newUuid = (now - st.st_mtime) > 3600;
        }
        else
            newUuid = true;
        if (!newUuid)
            newUuid = !readFile(uuidFilename, _uuid);
        if (!newUuid)
            newUuid = _uuid.empty();
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
#ifdef _WIN32
            _wutime(uuidFilename.c_str(), nullptr);
#else
            utime(uuidFilename.c_str(), nullptr);
#endif
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

        _ap = new APClient(_uuid, _game, uri, asset("cacert.pem").u8string());
        _ap->set_socket_connected_handler([this, slot, pw]() {
            auto lock = EventLock(_event);
            std::list<std::string> tags = {"PopTracker", "NoText"};
            if (_allowScout && !_allowSend)
                tags.push_back("HintGame");
            else if (!_allowSend)
                tags.push_back("Tracker");
            return _ap->ConnectSlot(slot, pw, 0b111, tags, {0, 5, 1});
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
            _itemIndex = 0;
            _slotData = slotData;
            _checkedLocations = _ap->get_checked_locations();
            _uncheckedLocations = _ap->get_missing_locations();
            onClear.emit(this, slotData);

            // update mtime of uuid file
            touchUUID();
        });
        _ap->set_slot_disconnected_handler([this]() {
            auto lock = EventLock(_event);
            _syncing = false;
            onStateChanged.emit(this, _ap->get_state());
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
                if (_syncing) {
                    // we started a sync to reset state -> replay onClear and onLocationChecked
                    _syncing = false;
                    onClear.emit(this, _slotData);
                    // create copy in case Lua checks locations
                    const auto checkedLocations = _checkedLocations;
                    for (int64_t location: checkedLocations) {
                        _uncheckedLocations.erase(location);
                        onLocationChecked.emit(this, location, _ap->get_location_name(location, _ap->get_game()));
                    }
                }
                onItem.emit(this, item.index, item.item, _ap->get_item_name(item.item, _ap->get_game()), item.player);
                _itemIndex++;
            }
        });
        _ap->set_location_checked_handler([this](const std::list<int64_t>& locations) {
            auto lock = EventLock(_event);
            for (int64_t location: locations) {
                _checkedLocations.insert(location);
                _uncheckedLocations.erase(location);
                onLocationChecked.emit(this, location, _ap->get_location_name(location, _ap->get_game()));
            }
        });
        _ap->set_location_info_handler([this](const std::list<APClient::NetworkItem>& scouts) {
            auto lock = EventLock(_event);
            for (const auto& scout: scouts) {
                onScout.emit(this, scout.location, _ap->get_location_name(scout.location, _ap->get_game()),
                        scout.item, _ap->get_item_name(scout.item, _ap->get_game()), scout.player);
            }
        });
        _ap->set_retrieved_handler([this](const std::map<std::string, json>& keys) {
            auto lock = EventLock(_event);
            for (const auto& pair: keys) {
                onRetrieved.emit(this, pair.first, pair.second);
            }
        });
        _ap->set_set_reply_handler([this](const std::string& key, const json& value, const json& old) {
            auto lock = EventLock(_event);
            onSetReply.emit(this, key, value, old);
        });
        _ap->set_room_update_handler([this]() {
            auto lock = EventLock(_event);
            onRoomUpdate.emit(this);
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
        _itemIndex = 0;
        _syncing = false;
        _slotData.clear();
        _checkedLocations.clear();
        _uncheckedLocations.clear();
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
#ifdef _WIN32
        _wutime(getConfigPath(_appname, UUID_FILENAME).c_str(), nullptr);
#else
        utime(getConfigPath(_appname, UUID_FILENAME).c_str(), nullptr);
#endif
    }

    int getPlayerNumber() const
    {
        return _ap ? _ap->get_player_number() : -1;
    }

    int getTeamNumber() const
    {
        return _ap ? _ap->get_team_number() : -1;
    }

    const std::string& getSeed() const
    {
        static std::string blank;
        return _ap ? _ap->get_seed() : blank;
    }

    const std::set<int64_t>& getCheckedLocations() const
    {
        return _checkedLocations;
    }

    const std::set<int64_t>& getMissingLocations() const
    {
        return _uncheckedLocations;
    }

    [[nodiscard]]
    std::string getPlayerAlias(int slot)
    {
        return _ap ? _ap->get_player_alias(slot) : "Unknown";
    }

    [[nodiscard]]
    std::string getPlayerGame(int slot)
    {
        return _ap ? _ap->get_player_game(slot) : "Unknown";
    }

    [[nodiscard]]
    std::string getItemName(int id, const std::string& game)
    {
        return _ap ? _ap->get_item_name(id, game) : "Unknown";
    }

    [[nodiscard]]
    std::string getLocationName(int id, const std::string& game)
    {
        return _ap ? _ap->get_location_name(id, game) : "Unknown";
    }

    bool SetNotify(const std::list<std::string>& keys)
    {
        return _ap ? _ap->SetNotify(keys) : false;
    }

    bool Get(const std::list<std::string>& keys)
    {
        return _ap ? _ap->Get(keys) : false;
    }

    bool Sync()
    {
        if (!_ap)
            return false;
        if (_itemIndex > 0 && !_syncing) {
            if (!_ap->Sync())
                return false;
            _syncing = true;
            _itemIndex = 0;
        } else if (!_syncing) {
            // never received an item, so we can immediately run onClear
            onClear.emit(this, _slotData);
            // create copy in case Lua checks locations
            const auto checkedLocations = _checkedLocations;
            for (int64_t location: checkedLocations) {
                _uncheckedLocations.erase(location);
                onLocationChecked.emit(this, location, _ap->get_location_name(location, _ap->get_game()));
            }
        }
        return true;
    }

    /// returns true if this instance is allowed to send Locations
    [[nodiscard]]
    bool allowSend() const
    {
        return _allowSend;
    }

    /// returns true if this instance is allowed to scout Locations
    [[nodiscard]]
    bool allowScout() const
    {
        return _allowScout;
    }

    /// returns true if locations were queued to be sent
    bool LocationChecks(const std::list<int64_t>& locations)
    {
        if (!_allowSend || !_ap)
            return false;
        if (!_ap->LocationChecks(locations))
            return false;
        for (auto location: locations) {
            auto it = _uncheckedLocations.find(location);
            if (it != _uncheckedLocations.end()) {
                _checkedLocations.insert(location);
                _uncheckedLocations.erase(it);
            }
        }
        return true;
    }

    /// returns true if locations were queued to be scouted
    bool LocationScouts(const std::list<int64_t>& locations, int createAsHint)
    {
        if (!_allowScout || !_ap)
            return false;
        return _ap->LocationScouts(locations, createAsHint);
    }

    /// Returns true if sending a client status update to the server.
    /// This is used to send the goal / win condition.
    bool StatusUpdate(APClient::ClientStatus status)
    {
        if (!_allowSend || !_ap)
            return false;
        return _ap->StatusUpdate(status);
    }

    Signal<const std::string&> onError;
    Signal<APClient::State> onStateChanged;
    Signal<> onRoomUpdate;
    Signal<const json&> onClear; // called when state has to be cleared, gives new slot_data
    Signal<int, int64_t, const std::string&, int> onItem; // index, item, item_name, player
    Signal<int64_t, const std::string&, int64_t, const std::string&, int> onScout; // location, location_name, item, item_name, target player
    Signal<int64_t, const std::string&> onLocationChecked; // location, location_name
    Signal<const json&> onBounced; // packet
    Signal<const std::string&, const json&> onRetrieved;
    Signal<const std::string&, const json&, const json&> onSetReply;

private:
    APClient* _ap = nullptr;
    std::string _appname;
    std::string _game;
    bool _allowSend;
    bool _allowScout;
    std::string _uuid;
    int _itemIndex = 0;
    std::set<int64_t> _checkedLocations;
    std::set<int64_t> _uncheckedLocations;
    int _event = 0;
    bool _scheduleDisconnect = false;
    json _slotData;
    bool _syncing = false;

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
