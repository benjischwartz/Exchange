#pragma once

#include <cstdint>
#include <expected>
#include <functional>
#include <string>

enum class Side { BUY, SELL };

// Example exchange interface
class IExchange {
   public:
    virtual ~IExchange() noexcept = default;

    // Add an order to the exchange
    // @return unique identifier for the order or descriptive error on failure
    virtual std::expected<std::uint64_t, std::string> AddOrder(
        const std::string &instrument, Side side, std::int64_t price,
        std::uint32_t quantity) = 0;

    // Remove an order from the exchange
    // @return success or failure
    virtual bool RemoveOrder(const std::string &instrument, Side side,
                             std::uint64_t orderId) = 0;

    // callback to indicate an order has been matched
    std::function<void(const std::string &instrument, std::uint64_t orderId,
                       std::int64_t tradedPrice, std::uint32_t tradedQuantity)>
        OrderTraded{};

    // callback to indicate the best price has changed for a given instrument
    std::function<void(const std::string &instrument, std::int64_t bidPrice,
                       std::uint32_t bidTotalQuantity, std::int64_t askPrice,
                       std::uint32_t askTotalQuantity)>
        BestPriceChanged{};
};
