/*
• Your task is to implement IExchange, an exchange capable of trading multiple
instruments with a time priority limit order book.
• Each time an order is added or removed that changes the best price or
quantity, call BestPriceChanged.
• Each time an order is added that trades with
an existing order, call OrderTraded. This should be for the existing order.
• Consider what constraints the exchange should have and when AddOrder should
return an error.
*/

/*
AddOrder error cases:
    1) Price is negative
    1) Quantity is <= 0
RemoveOrder error cases:
    1) orderId doesn't exist
 */

/*
Assumptions:
    * Number of orders won't exceed 2^64-1
    * New instruments get added to both sides
    * Order only trades if it trades completely (sufficient quantity)
 */

#include <cassert>
#include <deque>
#include <functional>
#include <iostream>
#include <map>
#include <string>
#include <unordered_map>

#include "IExchange.hpp"

class Exchange : public IExchange {
   public:
    Exchange() {}

    ~Exchange() override = default;

    std::expected<std::uint64_t, std::string> AddOrder(
        const std::string &instrument, Side side, std::int64_t price,
        std::uint32_t quantity) override;

    bool RemoveOrder(const std::string &instrument, Side side,
                     std::uint64_t orderId) override;

    void printInstrumentBooks(const std::string &instrument);

   private:
    struct Order {
        std::uint64_t orderId;
        std::uint32_t quantity;
    };

    using PriceT = uint64_t;

    template <typename Func>
    using PriceLevelT = std::map<PriceT, std::deque<Order>, Func>;

    template <typename Func>
    using OrderMapT =
        std::unordered_map<std::string /* instrument */, PriceLevelT<Func>>;

    OrderMapT<std::less<PriceT>> m_askMap;
    OrderMapT<std::greater<PriceT>> m_bidMap;

    std::unordered_map<uint64_t /* orderId */, PriceT> m_bidPriceMap;
    std::unordered_map<uint64_t /* orderId */, PriceT> m_askPriceMap;

    // For generating unique order IDs
    uint64_t m_bidOrderCount = 0;
    uint64_t m_askOrderCount = 0;
};

std::expected<std::uint64_t, std::string> Exchange::AddOrder(
    const std::string &instrument, Side side, std::int64_t price,
    std::uint32_t quantity)
{
    bool best_price_changed = false;
    auto process_orders = [&](auto &targetMap, auto &oppositeMap,
                              auto &&priceMatch) {
        auto price_iter = oppositeMap[instrument].begin();
        while (price_iter != oppositeMap[instrument].end() &&
               priceMatch(price_iter->first, price)) {
            auto &orders = price_iter->second;
            auto order_iter = orders.begin();
            while (order_iter != orders.end() && quantity > 0) {
                // Full trade
                if (order_iter->quantity <= quantity) {
                    OrderTraded(instrument, order_iter->orderId,
                                price_iter->first, order_iter->quantity);
                    quantity -= order_iter->quantity;
                    order_iter = orders.erase(order_iter);
                }
                // Partial trade
                else {
                    OrderTraded(instrument, order_iter->orderId,
                                price_iter->first, quantity);
                    order_iter->quantity -= quantity;
                    quantity = 0;
                }
            }
            if (orders.empty()) {
                // Remove price level if all orders are processed
                best_price_changed = true;
                price_iter = oppositeMap[instrument].erase(price_iter);
            }
            else {
                ++price_iter;
            }
        }
        if (quantity > 0) {
            if (side == Side::BUY) {
                m_bidPriceMap[m_bidOrderCount] = price;
            }
            else {
                m_askPriceMap[m_askOrderCount] = price;
            }
            targetMap[instrument][price].push_back(
                Order{side == Side::BUY ? m_bidOrderCount++ : m_askOrderCount++,
                      quantity});
        }
    };

    if (side == Side::BUY) {
        process_orders(m_bidMap, m_askMap, std::less_equal<PriceT>{});
    }
    else {
        process_orders(m_askMap, m_bidMap, std::greater_equal<PriceT>{});
    }
    return 0;
}

bool Exchange::RemoveOrder(const std::string &instrument, Side side,
                           std::uint64_t orderId)
{
    auto remove_order = [&](auto &targetMap, auto &targetPriceMap) {
        if (!targetMap.count(instrument) || !targetPriceMap.count(orderId)) {
            return false;
        }
        PriceT price = targetPriceMap[orderId];
        auto &price_level = targetMap[instrument][price];
        auto order_iter = price_level.begin();
        while (order_iter != price_level.end()) {
            if (order_iter->orderId == orderId) {
                price_level.erase(order_iter);
                return true;
            }
            ++order_iter;
        }
        return false;
    };

    if (side == Side::BUY) {
        return remove_order(m_bidMap, m_bidPriceMap);
    }
    else {
        return remove_order(m_askMap, m_askPriceMap);
    }
}

void Exchange::printInstrumentBooks(const std::string &instrument)
{
    std::cout << "Instrument=" << instrument << std::endl;
    auto printPriceLevel = [&](auto &targetMap) {
        if (!targetMap.count(instrument)) {
            return;
        }
        const auto &price_level = targetMap[instrument];
        for (const auto &[key, val] : price_level) {
            std::cout << "Price=" << key << "\n";
            for (const auto &order : val) {
                std::cout << "{id=" << order.orderId
                          << ", quantity=" << order.quantity << "} ";
            }
            std::cout << "\n";
        }
    };
    std::cout << "-----------------------------\n";
    std::cout << "Bids:\n";
    printPriceLevel(m_bidMap);
    std::cout << "-----------------------------\n";
    std::cout << "Asks:\n";
    printPriceLevel(m_askMap);
}

int main()
{
    Exchange ex;
    auto orderTraded = [](const std::string &instrument, std::int64_t orderId,
                          std::uint64_t tradedPrice,
                          std::int64_t tradedQuantity) {
        std::cout << "Order Traded: Instrument=" << instrument
                  << ", OrderId=" << orderId << ", TradedPrice=" << tradedPrice
                  << ", Traded Quantity=" << tradedQuantity << "\n";
    };
    auto bestPriceChanged =
        [](const std::string &instrument, std::int64_t bidPrice,
           std::uint32_t bidTotalQuantity, std::int64_t askPrice,
           std::int32_t askTotalQuantity) {
            std::cout << "Order Traded: Instrument=" << instrument
                      << ", BidPrice=" << bidPrice
                      << ", BidTotalQuantity=" << bidTotalQuantity
                      << ", AskPrice=" << askPrice
                      << ", AskTotalQuantity=" << askTotalQuantity << "\n";
        };
    ex.OrderTraded = orderTraded;
    ex.BestPriceChanged = bestPriceChanged;

    // Add some resting orders
    assert(ex.AddOrder("AAPL", Side::BUY, 69, 1000));
    assert(ex.AddOrder("AAPL", Side::SELL, 75, 750));
    assert(ex.AddOrder("AAPL", Side::BUY, 70, 1000));
    assert(ex.AddOrder("AAPL", Side::SELL, 76, 750));
    assert(ex.AddOrder("AAPL", Side::BUY, 68, 1000));
    assert(ex.AddOrder("AAPL", Side::SELL, 73, 750));

    // Partial order executes
    assert(ex.AddOrder("AAPL", Side::SELL, 70, 750));

    // Level clears
    assert(ex.AddOrder("AAPL", Side::SELL, 70, 750));

    // Clear a level, then partially another level
    assert(ex.AddOrder("AAPL", Side::BUY, 73, 1000));

    // Add on existing levels
    assert(ex.AddOrder("AAPL", Side::BUY, 69, 500));
    assert(ex.AddOrder("AAPL", Side::BUY, 69, 1000));
    assert(ex.AddOrder("AAPL", Side::BUY, 68, 1000));

    // Clear through two and a bit orders on level
    assert(ex.AddOrder("AAPL", Side::SELL, 69, 1750));

    // Add some more resting orders
    assert(ex.AddOrder("AAPL", Side::BUY, 69, 1000));
    assert(ex.AddOrder("AAPL", Side::BUY, 69, 1000));

    // Test removing one
    assert(ex.RemoveOrder("AAPL", Side::BUY, 6));

    ex.printInstrumentBooks("AAPL");
    std::cout << "\n";

    return 0;
}
