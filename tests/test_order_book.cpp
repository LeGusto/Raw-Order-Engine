#include "order_book.h"
#include <cassert>
#include <iostream>

int passed = 0;
int failed = 0;

void check(bool cond, const char *name)
{
    if (cond)
    {
        printf("  PASS: %s\n", name);
        passed++;
    }
    else
    {
        printf("  FAIL: %s\n", name);
        failed++;
    }
}

void test_empty_book()
{
    puts("== empty book ==");
    OrderBook book;
    check(!book.highest_bid().has_value(), "no highest bid");
    check(!book.lowest_ask().has_value(), "no lowest ask");
}

void test_single_ask()
{
    puts("== single ask ==");
    OrderBook book;
    auto rt = book.process_order(SIDE::ASK, 10, 100, 1);
    check(std::holds_alternative<Order>(rt), "returns order (no match)");
    check(book.lowest_ask().has_value(), "ask exists");
    check(book.lowest_ask()->price == 100, "ask price correct");
    check(book.lowest_ask()->quantity == 10, "ask quantity correct");
    check(!book.highest_bid().has_value(), "no bids");
}

void test_single_bid()
{
    puts("== single bid ==");
    OrderBook book;
    auto rt = book.process_order(SIDE::BID, 5, 200, 2);
    check(std::holds_alternative<Order>(rt), "returns order (no match)");
    check(book.highest_bid().has_value(), "bid exists");
    check(book.highest_bid()->price == 200, "bid price correct");
    check(book.highest_bid()->quantity == 5, "bid quantity correct");
    check(!book.lowest_ask().has_value(), "no asks");
}

void test_no_match_spread()
{
    puts("== no match (bid < ask) ==");
    OrderBook book;
    book.process_order(SIDE::ASK, 10, 100, 1);
    auto rt = book.process_order(SIDE::BID, 5, 50, 2);
    check(std::holds_alternative<Order>(rt), "no match when bid < ask");
    check(book.lowest_ask()->price == 100, "ask still there");
    check(book.highest_bid()->price == 50, "bid still there");
}

void test_exact_match()
{
    puts("== exact match ==");
    OrderBook book;
    book.process_order(SIDE::ASK, 10, 100, 1);
    auto rt = book.process_order(SIDE::BID, 10, 100, 2);
    check(std::holds_alternative<std::vector<Match>>(rt), "returns matches");
    auto matches = std::get<std::vector<Match>>(rt);
    check(matches.size() == 1, "one match");
    check(matches[0].quantity == 10, "matched full quantity");
    check(!book.lowest_ask().has_value(), "ask consumed");
    check(!book.highest_bid().has_value(), "bid consumed");
}

void test_partial_match_ask_remaining()
{
    puts("== partial match (ask remaining) ==");
    OrderBook book;
    book.process_order(SIDE::ASK, 10, 100, 1);
    auto rt = book.process_order(SIDE::BID, 3, 100, 2);
    check(std::holds_alternative<std::vector<Match>>(rt), "returns matches");
    auto matches = std::get<std::vector<Match>>(rt);
    check(matches[0].quantity == 3, "matched 3");
    check(book.lowest_ask()->quantity == 7, "ask has 7 remaining");
    check(!book.highest_bid().has_value(), "bid consumed");
}

void test_partial_match_bid_remaining()
{
    puts("== partial match (bid remaining) ==");
    OrderBook book;
    book.process_order(SIDE::ASK, 3, 100, 1);
    auto rt = book.process_order(SIDE::BID, 10, 100, 2);
    check(std::holds_alternative<std::vector<Match>>(rt), "returns matches");
    auto matches = std::get<std::vector<Match>>(rt);
    check(matches[0].quantity == 3, "matched 3");
    check(!book.lowest_ask().has_value(), "ask consumed");
    check(book.highest_bid()->quantity == 7, "bid has 7 remaining");
}

void test_bid_higher_than_ask()
{
    puts("== bid higher than ask ==");
    OrderBook book;
    book.process_order(SIDE::ASK, 5, 100, 1);
    auto rt = book.process_order(SIDE::BID, 5, 150, 2);
    check(std::holds_alternative<std::vector<Match>>(rt), "matches when bid > ask");
    check(!book.lowest_ask().has_value(), "ask consumed");
    check(!book.highest_bid().has_value(), "bid consumed");
}

void test_multiple_asks_price_priority()
{
    puts("== multiple asks (price priority) ==");
    OrderBook book;
    book.process_order(SIDE::ASK, 5, 200, 1);
    book.process_order(SIDE::ASK, 5, 100, 2);
    check(book.lowest_ask()->price == 100, "lowest ask is cheapest");
    auto rt = book.process_order(SIDE::BID, 5, 100, 3);
    check(std::holds_alternative<std::vector<Match>>(rt), "matches cheapest ask");
    check(book.lowest_ask()->price == 200, "expensive ask remains");
}

void test_multiple_bids_price_priority()
{
    puts("== multiple bids (price priority) ==");
    OrderBook book;
    book.process_order(SIDE::BID, 5, 50, 1);
    book.process_order(SIDE::BID, 5, 100, 2);
    check(book.highest_bid()->price == 100, "highest bid is most expensive");
    auto rt = book.process_order(SIDE::ASK, 5, 100, 3);
    check(std::holds_alternative<std::vector<Match>>(rt), "matches highest bid");
    check(book.highest_bid()->price == 50, "cheaper bid remains");
}

void test_cancel_order()
{
    puts("== cancel order ==");
    OrderBook book;
    auto rt = book.process_order(SIDE::ASK, 10, 100, 1);
    auto order = std::get<Order>(rt);
    auto cancelled = book.cancel_order(order.id);
    check(cancelled.has_value(), "cancel returns order");
    check(cancelled->id == order.id, "cancelled correct order");
    check(!book.lowest_ask().has_value(), "ask removed from book");
}

void test_cancel_nonexistent()
{
    puts("== cancel nonexistent ==");
    OrderBook book;
    auto cancelled = book.cancel_order(99999);
    check(!cancelled.has_value(), "returns nullopt");
}

void test_multiple_matches()
{
    puts("== multiple matches ==");
    OrderBook book;
    book.process_order(SIDE::ASK, 5, 100, 1);
    book.process_order(SIDE::ASK, 5, 110, 2);
    auto rt = book.process_order(SIDE::BID, 8, 110, 3);
    check(std::holds_alternative<std::vector<Match>>(rt), "returns matches");
    auto matches = std::get<std::vector<Match>>(rt);
    check(matches.size() == 2, "two matches");
    check(matches[0].quantity == 5, "first match qty 5");
    check(matches[1].quantity == 3, "second match qty 3");
    check(!book.lowest_ask().has_value() || book.lowest_ask()->quantity == 2, "remaining ask correct");
}

void test_get_orders()
{
    puts("== get orders ==");
    OrderBook book;
    book.process_order(SIDE::ASK, 5, 100, 1);
    book.process_order(SIDE::BID, 3, 50, 1);
    book.process_order(SIDE::ASK, 7, 200, 2);
    auto orders = book.get_orders(1);
    check(orders.size() == 2, "customer 1 has 2 orders");
}

int main()
{
    test_empty_book();
    test_single_ask();
    test_single_bid();
    test_no_match_spread();
    test_exact_match();
    test_partial_match_ask_remaining();
    test_partial_match_bid_remaining();
    test_bid_higher_than_ask();
    test_multiple_asks_price_priority();
    test_multiple_bids_price_priority();
    test_cancel_order();
    test_cancel_nonexistent();
    test_multiple_matches();
    test_get_orders();

    printf("\n%d passed, %d failed\n", passed, failed);
    return failed > 0 ? 1 : 0;
}
