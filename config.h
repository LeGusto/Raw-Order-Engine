#pragma once

#include <string>

constexpr const char *PORT = "8080"; // anything below 1024 is reserved, valid up to 65535
constexpr const char *HOST = "localhost";
constexpr const int BACKLOG = 20;
constexpr const bool DEBUG = true;
