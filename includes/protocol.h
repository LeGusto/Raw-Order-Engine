#pragma once

#include <cstdint>
#include <tuple>
#include "order_book.h"

enum class MessageType : uint8_t
{
    SUBMIT_ORDER = 0x01,
    CANCEL_ORDER = 0x02,
    ORDER_ACK = 0x03,
    CANCEL_ACK = 0x04,
    MATCH = 0x05,
    REJECT = 0x06,
    GET_ORDERS = 0x07,
    ORDERS_LIST = 0x08,
};

constexpr uint32_t MAX_REQUEST_SIZE = 128000;

struct SubmitOrderPayload
{
    uint32_t user_id = 0;
    Side side = Side::ASK;
    uint32_t price = 0;
    uint32_t quantity = 0;

    auto fields() const
    {
        return std::tie(user_id, side, price, quantity);
    }

    auto fields()
    {
        return std::tie(user_id, side, price, quantity);
    }
};
