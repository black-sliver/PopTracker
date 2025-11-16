#pragma once

#include <string>
#include <nlohmann/json.hpp>
#include "release.hpp"


namespace gh::api {
    class Releases final {
        using json = nlohmann::json;
    public:
        class iterator final {
        friend class Releases;
        public:
            iterator operator++()
            {
                ++_pos;
                return *this;
            }

            friend bool operator== (const iterator& a, const iterator& b)
            {
                return &a._data == &b._data && a._pos == b._pos;
            }

            friend bool operator!= (const iterator& a, const iterator& b)
            {
                return !(a == b);
            }

            Release operator*() const
            {
                Release r;
                from_json(_data.at(_pos), r);
                return r;
            }

            iterator(const iterator& other) = default;
            iterator(iterator&& other) = default;

        private:
            explicit iterator(json& data, const size_t pos = 0)
                : _data(data), _pos(pos)
            {
            }

            json& _data;
            size_t _pos;
        };

        explicit Releases(const std::string& data)
            : _data(json::parse(data))
        {
            if (!_data.is_array())
                throw std::invalid_argument("data must be a json array");
        }

        iterator begin()
        {
            return iterator{_data};
        }

        iterator end()
        {
            return iterator{_data, _data.size()};
        }

    private:
        json _data;
    };
} // gh::api
