#pragma once

#include <chrono>
#include <string_view>
using std::chrono::milliseconds;
using namespace std::chrono_literals;

static constexpr milliseconds DEFAULT_MAC_TIMEOUT = 30'000ms;
static constexpr milliseconds DEFAULT_SENT_PACKET_TIMEOUT = 300s;
static constexpr milliseconds DEFAULT_SESSION_TIMEOUT = 30'000ms;
static constexpr std::string_view DEFAULT_HOSTNAME = "Switch";

static constexpr milliseconds MAC_UPDATE_TIMER = 200ms;
static constexpr milliseconds SENT_PACKETS_TIMER = 300ms;
static constexpr milliseconds INTERFACE_UPDATE_TIMER = 1'000ms;
static constexpr milliseconds SESSION_UPDATE_TIMER = 1'000ms;
static constexpr milliseconds UI_REFRESH_TIMER = 500ms;
static constexpr milliseconds STATS_REFRESH_TIMER = 500ms;

static constexpr std::string_view REST_USERNAME = "root";
static constexpr std::string_view REST_PASSWORD = "root";
static constexpr int32_t TOKEN_LENGTH = 32;

