#pragma once

#include <thread>
#include <mutex>
#include <vector>
#include <string>
#include <cstdio>
#include <utility>
#include <deque>
#include <chrono>

#ifdef _WIN32
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif
#endif

#ifndef ASIO_STANDALONE
#define ASIO_STANDALONE
#endif

#include <asio.hpp>
#include <asio/ts/buffer.hpp>

#include <nlohmann/json.hpp>

#define LUACONNECTOR_ASYNC
