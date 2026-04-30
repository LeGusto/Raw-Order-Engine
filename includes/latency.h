
#include <vector>
#include <string>
#include <string_view>
#include <fstream>
#include <ctime>
#include "enum_name.h"
#include <filesystem>
#include <numeric>
#include <execution>

#ifndef PROJECT_ROOT
#define PROJECT_ROOT "."
#endif

struct Entry
{
    std::string op_name = "";
    uint64_t ns = 0;

    Entry(std::string_view _op_name, uint64_t _ns) : op_name(_op_name), ns(_ns) {};
};

class LatencyTracker
{
private:
    std::vector<Entry> entries;
    size_t n = 0;
    std::unordered_map<std::string, std::vector<uint64_t>> buckets;

    long double avg()
    {
        uint64_t tot = std::transform_reduce(std::execution::par_unseq, entries.begin(), entries.end(), uint64_t(0), std::plus<>{}, [](const Entry &e)
                                             { return e.ns; });

        return (long double)tot / entries.size();
    }

    uint64_t min()
    {
        return entries[0].ns;
    }

    uint64_t max()
    {
        return entries.rbegin()->ns;
    }

    uint64_t pxx(long double percentile)
    {
        long double div = (entries.size() * percentile) / 100;
        size_t idx = static_cast<size_t>(div);
        return entries[idx].ns;
    }

public:
    LatencyTracker(size_t _n) : n(_n)
    {
        entries.reserve(n);
    }

    void insert_entry(std::string_view op_name, int ns)
    {
        if (entries.size() < n)
        {
            entries.emplace_back(op_name, ns);
        }
    }

    void partition_entries()
    {
        for (auto &v : entries)
        {
            buckets[v.op_name].push_back(v.ns);
        }
    }

    void dump_entries()
    {
        sort(entries.begin(), entries.end(), [](const Entry &a, const Entry &b)
             { return a.ns < b.ns; });

        std::time_t t = std::time(nullptr);
        char buf[32];
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&t));

        std::filesystem::path log_dir = std::filesystem::path(PROJECT_ROOT) / "logs";
        std::ofstream out(log_dir / std::format("bench {}.txt", buf));

        std::print(out,
                   "Average: {}\n"
                   "Min: {}\n"
                   "Max: {}\n"
                   "P50: {}\n"
                   "P95: {}\n"
                   "P99: {}\n"
                   "P99.99: {}\n",
                   avg(), min(), max(), pxx(50), pxx(95), pxx(99), pxx(99.99));
    }
};