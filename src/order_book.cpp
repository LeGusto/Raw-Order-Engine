#include "order_book.h"
#include <optional>
#include <iostream>
#include <format>
#include <string>

std::optional<Order> OrderBook::highest_bid()
{
    if (this->bidMap.empty())
        return std::nullopt;

    return *bidMap.begin()->second.begin();
}

std::optional<Order> OrderBook::lowest_ask()
{
    if (this->askMap.empty())
        return std::nullopt;

    return *askMap.begin()->second.begin();
}

void OrderBook::remove_order_refs(uint32_t orderID)
{
    auto &nav = orderIDMap.at(orderID);
    customerIDMap[nav.order_it->customerID].erase(nav.customer_it);
    if (customerIDMap[nav.order_it->customerID].empty())
        customerIDMap.erase(nav.order_it->customerID);
    orderIDMap.erase(orderID);
}

std::vector<Match> OrderBook::match_orders()
{
    std::vector<Match> res;
    while (!askMap.empty() && !bidMap.empty())
    {
        auto ptr_a = &askMap.begin()->second;
        auto ptr_b = &bidMap.begin()->second;
        if (ptr_a->begin()->price <= ptr_b->begin()->price)
        {
            uint32_t qty = std::min(ptr_a->begin()->quantity, ptr_b->begin()->quantity);

            res.push_back({*ptr_a->begin(), *ptr_b->begin(), qty});

            if (ptr_a->begin()->quantity == qty)
            {
                remove_order_refs(ptr_a->begin()->id);
                ptr_a->pop_front();
                if (ptr_a->empty())
                    askMap.erase(askMap.begin());
            }
            else
            {
                ptr_a->begin()->quantity -= qty;
            }

            if (ptr_b->begin()->quantity == qty)
            {
                remove_order_refs(ptr_b->begin()->id);
                ptr_b->pop_front();
                if (ptr_b->empty())
                    bidMap.erase(bidMap.begin());
            }
            else
                ptr_b->begin()->quantity -= qty;
        }
        else
            break;
    }

    return res;
}

std::variant<std::vector<Match>, Order> OrderBook::process_order(Side side, uint32_t quantity, uint32_t price, uint32_t customerID)
{
    Order order{side, quantity, price, customerID};
    std::list<Order>::iterator order_it;
    if (side == Side::ASK)
    {
        askMap[order.price].push_back(order);
        order_it = askMap[order.price].end();
    }
    else if (side == Side::BID)
    {
        bidMap[order.price].push_back(order);
        order_it = bidMap[order.price].end();
    }

    advance(order_it, -1);
    customerIDMap[customerID].push_back(order_it);

    std::list<std::list<Order>::iterator>::iterator customer_it = customerIDMap[customerID].end();
    advance(customer_it, -1);

    mapNavigation entry = {order_it, customer_it, order};
    orderIDMap.insert({order.id, entry});

    std::vector<Match> matches = match_orders();
    if (!matches.empty())
        return matches;

    return order;
}

std::optional<Order> OrderBook::cancel_order(uint32_t orderID)
{
    auto found = orderIDMap.find(orderID);
    if (found == orderIDMap.end())
        return std::nullopt;

    mapNavigation item = found->second;

    Order rt = *item.order_it;

    remove_order_refs(orderID);

    if (item.side == Side::BID)
    {
        bidMap[item.price].erase(item.order_it);
        if (bidMap[item.price].empty())
            bidMap.erase(item.price);
    }
    else if (item.side == Side::ASK)
    {
        askMap[item.price].erase(item.order_it);
        if (askMap[item.price].empty())
            askMap.erase(item.price);
    }

    return rt;
}

std::vector<Order> OrderBook::get_orders(uint32_t customerID)
{
    std::vector<Order> rt;
    if (customerIDMap.find(customerID) == customerIDMap.end())
        return rt;

    rt.reserve(customerIDMap[customerID].size());

    auto it = customerIDMap[customerID].begin();
    while (it != customerIDMap[customerID].end())
    {
        rt.push_back(*(*it));
        advance(it, 1);
    }
    return rt;
}

std::string OrderBook::check_invariants() const
{
    // best ask must be > best bid
    if (!askMap.empty() && !bidMap.empty())
    {
        uint32_t best_ask = askMap.begin()->first;
        uint32_t best_bid = bidMap.begin()->first;
        if (best_ask <= best_bid)
            return std::format("crossed book: best_ask={} <= best_bid={}", best_ask, best_bid);
    }

    size_t orders_in_books = 0;

    // each ask price level: non-empty, all orders match
    for (const auto &[price, lst] : askMap)
    {
        if (lst.empty())
            return std::format("empty ask price level at {}", price);
        for (const auto &o : lst)
        {
            if (o.price != price)
                return std::format("ask order id={} has price {} but is at level {}", o.id, o.price, price);
            if (o.side != Side::ASK)
                return std::format("ask order id={} has side != ASK", o.id);
            ++orders_in_books;
        }
    }

    // same for bids
    for (const auto &[price, lst] : bidMap)
    {
        if (lst.empty())
            return std::format("empty bid price level at {}", price);
        for (const auto &o : lst)
        {
            if (o.price != price)
                return std::format("bid order id={} has price {} but is at level {}", o.id, o.price, price);
            if (o.side != Side::BID)
                return std::format("bid order id={} has side != BID", o.id);
            ++orders_in_books;
        }
    }

    // orderIDMap size matches total orders in books
    if (orderIDMap.size() != orders_in_books)
        return std::format("orderIDMap.size()={} but books contain {} orders",
                           orderIDMap.size(), orders_in_books);

    // each orderIDMap entry's iterator points to a real order with matching info
    for (const auto &[id, nav] : orderIDMap)
    {
        if (nav.order_it->id != id)
            return std::format("orderIDMap[{}] iterator points to order id={}", id, nav.order_it->id);
        if (nav.order_it->price != nav.price)
            return std::format("orderIDMap[{}] price mismatch: {} vs {}", id, nav.order_it->price, nav.price);
        if (nav.order_it->side != nav.side)
            return std::format("orderIDMap[{}] side mismatch", id);
    }

    // customerIDMap: no empty entries, total iterators == orderIDMap.size,
    // each iterator's customerID matches the map key
    size_t orders_in_customers = 0;
    for (const auto &[cust, lst] : customerIDMap)
    {
        if (lst.empty())
            return std::format("empty customer entry for customer {}", cust);
        for (const auto &it : lst)
        {
            if (it->customerID != cust)
                return std::format("customerIDMap[{}] iterator points to order owned by {}",
                                   cust, it->customerID);
            ++orders_in_customers;
        }
    }
    if (orders_in_customers != orderIDMap.size())
        return std::format("customerIDMap total={} but orderIDMap.size()={}",
                           orders_in_customers, orderIDMap.size());

    return "";
}