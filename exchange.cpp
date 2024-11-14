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

#include <deque>
#include <map>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "IExchange.hpp"

class Exchange : IExchange {
   public:
    ~Exchange() override;

    std::expected<std::uint64_t, std::string> AddOrder(
        const std::string& instrument, Side side, std::int64_t price,
        std::uint32_t quantity) override
    {
        if (price <= 0)
            return std::unexpected("Price must be positive");
        else if (quantity <= 0)
            return std::unexpected("Quantity must be positive");

        if (side == Side::BUY) {
            auto it = m_bidMap.find(instrument);

            // NEW INSTRUMENT
            if (it == m_bidMap.end()) {
                m_bidMap[instrument][price].first.push_back(
                    Order{m_bidOrderCount, price, quantity});
                m_askMap[instrument];  // insert new entry on ask side
            }
            else {
                auto price_iter = m_askMap[instrument].begin();
                // Can trade at a given price
                while (price_iter != m_askMap[instrument].end() &&
                       price >= price_iter->first) {
                    auto order_iter = price_iter->second.first.begin();
                    while (order_iter != price_iter->second.first.end() &&
                           quantity >= order_iter->quantity) {
                        OrderTraded(instrument, order_iter->orderId,
                                    price_iter->first, order_iter->quantity);
                        quantity -= order_iter->quantity;
                        // Decrement total quantity for that price
                        price_iter->second.second -= order_iter->quantity;
                        order_iter = price_iter->second.first.erase(order_iter);
                    }
                    // We have cleared a price-level
                    if (order_iter == price_iter->second.first.end()) {
                        m_askMap[instrument].erase(price_iter);
                        BestPriceChanged(instrument,
                                         m_bidMap[instrument].begin()->first, 
                                         m_bidMap[instrument].begin()->second.second,
                                         m_askMap[instrument].begin()->first,
                                         m_askMap[instrument].begin()->second.second
                                         );
                    }
                }
            }
            return m_bidOrderCount++;
        }
        else {
            auto it = m_askMap.find(instrument);

            // NEW INSTRUMENT
            if (it == m_askMap.end()) {
                m_askMap[instrument][price].first.push_back(
                    Order{m_askOrderCount, price, quantity});
                m_bidMap[instrument];  // insert new entry on bid side
            }
            else {
            }
            return m_askOrderCount++;
        }
    };

    bool RemoveOrder(const std::string& instrument, Side side,
                     std::uint64_t orderId) override
    {
        return 0;
    }

   private:
    struct Order {
        std::uint64_t orderId;
        std::int64_t price; /* Might not need this */
        std::uint32_t quantity;
    };

    using PriceT = uint64_t;

    // Prices sorted in descending order
    using BidPriceTimeT = std::map<
        PriceT,
        std::pair<std::deque<Order>, uint32_t /* best price total quantity */>,
        std::greater<>>;

    // Prices sorted in ascending order
    using AskPriceTimeT =
        std::map<PriceT, std::pair<std::deque<Order>,
                                   uint32_t /* best price total quantity */>>;

    std::unordered_map<std::string /* instrument */, BidPriceTimeT> m_bidMap;
    std::unordered_map<std::string /* instrument */, AskPriceTimeT> m_askMap;

    std::unordered_set<std::uint64_t> m_bidOrderIds;
    std::unordered_set<std::uint64_t> m_askOrderIds;

    uint64_t m_bidOrderCount = 0;
    uint64_t m_askOrderCount = 0;
};
