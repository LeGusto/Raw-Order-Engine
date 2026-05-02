#pragma once

#include <vector>
#include <string>
#include <string_view>
#include <unordered_map>
#include <cstdint>

class LatencyTracker
{
private:
    size_t n = 0;
    size_t total_inserted = 0;
    std::unordered_map<std::string, std::vector<uint64_t>> buckets;

public:
    explicit LatencyTracker(size_t per_bucket_cap);

    void insert_entry(std::string_view op_name, uint64_t ns);
    void dump_entries();
};
