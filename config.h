#pragma once

#include <string>

constexpr const char *PORT = "8089"; // anything below 1024 is reserved, valid up to 65535
constexpr const char *HOST = "localhost";
constexpr const int BACKLOG = 20;
constexpr const bool DEBUG = false;
constexpr const int AI_FAMILY = AF_INET6;
constexpr const int AI_SOCKTYPE = SOCK_STREAM;
constexpr const int AI_FLAGS = AI_PASSIVE; // fill in with own IP
constexpr const size_t UDP_PACKET_SIZE = 1024;
constexpr const bool BLOCKING = false;
constexpr const int NUM_USERS = 10;
constexpr const int USER_REQUESTS = 100000;
constexpr const int LOG_ENTRIES = 2000000;
