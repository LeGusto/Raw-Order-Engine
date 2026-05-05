#include "order_book.h"

#include <chrono>
#include <cstdint>
#include <print>
#include <random>
#include <vector>

namespace
{
    constexpr int ITERATIONS = 20'000;
    constexpr int NUM_CUSTOMERS = 10;
    constexpr uint32_t PRICE_MIN = 50;
    constexpr uint32_t PRICE_MAX = 200;
    constexpr uint32_t QTY_MIN = 1;
    constexpr uint32_t QTY_MAX = 100;
    constexpr int CANCEL_PCT = 25;

    void check(const OrderBook &book, int iter, const char *op)
    {
        auto err = book.check_invariants();
        if (!err.empty())
        {
            std::println(stderr, "FAIL at iter {} after {}: {}", iter, op, err);
            std::exit(1);
        }
    }
}

int main()
{
    OrderBook book;
    std::mt19937 rng(0xCAFEBABE);
    std::uniform_int_distribution<int> side_dist(0, 1);
    std::uniform_int_distribution<uint32_t> price_dist(PRICE_MIN, PRICE_MAX);
    std::uniform_int_distribution<uint32_t> qty_dist(QTY_MIN, QTY_MAX);
    std::uniform_int_distribution<int> cust_dist(0, NUM_CUSTOMERS - 1);
    std::uniform_int_distribution<int> action_dist(0, 99);

    std::vector<uint32_t> active_ids;
    active_ids.reserve(ITERATIONS);

    auto t0 = std::chrono::steady_clock::now();

    for (int i = 0; i < ITERATIONS; ++i)
    {
        bool do_cancel = action_dist(rng) < CANCEL_PCT && !active_ids.empty();

        if (do_cancel)
        {
            std::uniform_int_distribution<size_t> idx_dist(0, active_ids.size() - 1);
            size_t idx = idx_dist(rng);
            uint32_t id = active_ids[idx];
            active_ids[idx] = active_ids.back();
            active_ids.pop_back();

            book.cancel_order(id);
            check(book, i, "cancel");
        }
        else
        {
            Side side = static_cast<Side>(side_dist(rng));
            uint32_t price = price_dist(rng);
            uint32_t qty = qty_dist(rng);
            uint32_t cust = cust_dist(rng);

            auto res = book.process_order(side, qty, price, cust);

            if (std::holds_alternative<Order>(res))
                active_ids.push_back(std::get<Order>(res).id);

            check(book, i, "submit");
        }
    }

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - t0);

    std::println("OK: {} iterations, {} resting orders, {}ms",
                 ITERATIONS, active_ids.size(), elapsed.count());
    return 0;
}
