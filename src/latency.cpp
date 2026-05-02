#include "latency.h"

#include <chrono>
#include <ctime>
#include <filesystem>
#include <format>
#include <fstream>
#include <print>
#include <string>

#ifndef PROJECT_ROOT
#define PROJECT_ROOT "."
#endif

LatencyTracker::LatencyTracker(size_t per_bucket_cap) : n(per_bucket_cap)
{
}

void LatencyTracker::insert_entry(std::string_view op_name, uint64_t ns)
{
    auto &b = buckets[std::string(op_name)];
    if (b.capacity() == 0)
        b.reserve(n);
    if (b.size() < n)
    {
        b.push_back(ns);
        ++total_inserted;
    }
}

void LatencyTracker::dump_entries()
{
    if (total_inserted == 0)
        return;

    std::time_t t = std::time(nullptr);
    char ts[32];
    std::strftime(ts, sizeof(ts), "%Y%m%dT%H%M%S", std::localtime(&t));

    std::filesystem::path run_dir =
        std::filesystem::path(PROJECT_ROOT) / "logs" / std::format("run_{}", ts);
    std::filesystem::create_directories(run_dir);

    for (auto &[name, vec] : buckets)
    {
        if (vec.empty())
            continue;
        std::ofstream out(run_dir / (name + ".txt"));
        if (!out)
            continue;
        for (uint64_t ns : vec)
            std::println(out, "{}", ns);
    }
}
