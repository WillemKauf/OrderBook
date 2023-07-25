#ifndef ORDER_BOOK_H
#define ORDER_BOOK_H

/////////////////
/// std
/////////////////
#include <chrono>
#include <functional>
#include <list>
#include <map>
#include <string>
#include <unordered_map>
#include<iostream>
// Defining types
using UserId_type      = int;
using Symbol_type      = std::string;
using OrderId_type     = int;
using Price_type       = int;
using Quantity_type    = int;
using UserOrderId_type = int;

struct Order {
  UserId_type userId       = -1;
  OrderId_type userOrderId = -1;
  enum class OrderType {
    BUY,
    SELL,
    CANCEL,
    FLUSH
  } orderType;

  Symbol_type symbol     = "null";
  Price_type price       = -1;
  Quantity_type quantity = -1;

  template <typename Archive>
  void serialize(Archive& archive) {
    archive& userId& userOrderId& orderType& symbol& price& quantity;
  }
};

std::string to_string(Order::OrderType type) {
  switch (type) {
    case (Order::OrderType::BUY):
      return "BUY";
    case (Order::OrderType::SELL):
      return "SELL";
    case (Order::OrderType::CANCEL):
      return "CANCEL";
    case (Order::OrderType::FLUSH):
      return "FLUSH";
    default:
      throw std::runtime_error("Invalid OrderType");
  }
}

std::string to_string(Order order) {
  return to_string(order.orderType) + " " + std::to_string(order.userId) + " " + order.symbol +
         " " + std::to_string(order.price) + " " + std::to_string(order.quantity) + " " +
         std::to_string(order.userOrderId);
}

class OrderBook {
public:
  using List_type = std::list<Order>;

  struct Limit {
    int totalQuantity = 0;
    List_type list;
  };

  struct OrderIt {
    List_type* listPtr;
    List_type::iterator it;
  };

  using OrderMap_type = std::unordered_map<OrderId_type, OrderIt>;
  using Map_type      = std::map<Price_type, Limit>;  // BST

private:
  Map_type buySide_;
  Map_type sellSide_;

  std::unordered_map<Price_type, Limit*> priceMap_;

  Map_type::reverse_iterator TOBBuySide_ = buySide_.rbegin();
  Map_type::iterator TOBSellSide_        = sellSide_.begin();

  void UpdateTOBBuySide() {
    for (TOBBuySide_ = buySide_.rbegin();
         TOBBuySide_ != buySide_.rend() && TOBBuySide_->second.totalQuantity == 0; ++TOBBuySide_) {
    }
  }

  void UpdateTOBSellSide() {
    for (TOBSellSide_ = sellSide_.begin();
         TOBSellSide_ != sellSide_.end() && TOBSellSide_->second.totalQuantity == 0;
         ++TOBSellSide_) {
    }
  }

public:
  std::vector<std::string> BuyOrder(Order order, OrderMap_type& orderIdMap) {
    std::vector<std::string> logVec;
    {
      std::stringstream logStr;
      logStr << "A, " << order.userId << ", " << order.userOrderId;
      logVec.push_back(logStr.str());
    }
    // Check sell side.
    bool updateTOB = false;
    while (TOBSellSide_ != sellSide_.end() && order.quantity > 0) {
      // We need to first check if the TOBSellSide <= our order.price, or if we are placing a market
      // order.
      const auto& sellPrice = TOBSellSide_->first;
      if (sellPrice <= order.price || order.price == 0) {
        // We can trade.
        const auto salePrice = sellPrice;
        auto& sellLimit      = TOBSellSide_->second;
        auto& sellList       = sellLimit.list;
        auto it              = sellList.begin();
        while (it != sellList.end() && order.quantity > 0) {
          auto& sellOrder = *it;
          if (sellOrder.userId == order.userId) {
            // We can't trade with ourselves!
            ++it;
            continue;
          }
          const auto saleQuantity = std::min(order.quantity, sellOrder.quantity);
          {
            std::stringstream logStr;
            logStr << "T, " << order.userId << ", " << order.userOrderId << ", " << sellOrder.userId
                   << ", " << sellOrder.userOrderId << ", " << salePrice << ", " << saleQuantity;
            logVec.push_back(logStr.str());
          }
          sellLimit.totalQuantity -= saleQuantity;
          order.quantity -= saleQuantity;
          if (saleQuantity == sellOrder.quantity) {
            orderIdMap.erase(sellOrder.userOrderId);
            it = sellList.erase(it);
          }
        }
        updateTOB = true;

        // We have taken all of the TOB on sell side. Update.
        if (it == sellList.end()) {
          UpdateTOBSellSide();
        }
      } else {
        break;
      }
    }

    if (updateTOB) {
      const auto isEliminated = (TOBSellSide_ == sellSide_.end());
      std::string TOBPrice    = (isEliminated) ? "-" : std::to_string(TOBSellSide_->first);
      std::string TOBQuantity =
          (isEliminated) ? "-" : std::to_string(TOBSellSide_->second.totalQuantity);
      std::stringstream logStr;
      logStr << "B, S, " << TOBPrice << ", " << TOBQuantity;
      logVec.push_back(logStr.str());
    }

    // Return if we have nothing left to do with this order, or if it is a market order.
    if (order.quantity <= 0 || order.price == 0) {
      return logVec;
    }

    // MAKE THIS O(1) FOR EVERY ENTRY AFTER THE FIRST O(log n) ENTRY.
    const auto inPriceMap = priceMap_.contains(order.price);
    auto& limit           = (inPriceMap) ? *priceMap_[order.price] : buySide_[order.price];
    if (!inPriceMap) {
      priceMap_[order.price] = &limit;
    }
    auto& list = limit.list;

    limit.totalQuantity += order.quantity;
    UpdateTOBBuySide();
    if (order.price == TOBBuySide_->first) {
      std::stringstream logStr;
      logStr << "B, B, " << order.price << ", " << TOBBuySide_->second.totalQuantity;
      logVec.push_back(logStr.str());
    }
    const auto userOrderId = order.userOrderId;
    list.push_back(std::move(order));
    orderIdMap.insert_or_assign(userOrderId, OrderIt{&list, std::prev(list.end())});
    return logVec;
  }

  std::vector<std::string> SellOrder(Order order, OrderMap_type& orderIdMap) {
    std::vector<std::string> logVec;
    {
      std::stringstream logStr;
      logStr << "A, " << order.userId << ", " << order.userOrderId;
      logVec.push_back(logStr.str());
    }
    // Check buy side.
    bool updateTOB = false;
    while (TOBBuySide_ != buySide_.rend() && order.quantity > 0) {
      // We need to first check if the TOBBuySide >= our order.price (will still work if we are
      // placing a market order)
      const auto& buyPrice = TOBBuySide_->first;
      if (buyPrice >= order.price) {
        // We can trade.
        const auto salePrice = (order.price == 0) ? buyPrice : std::min(buyPrice, order.price);
        auto& buyLimit       = TOBBuySide_->second;
        auto& buyList        = buyLimit.list;
        auto it              = buyList.rbegin();
        while (it != buyList.rend() && order.quantity > 0) {
          auto& buyOrder = *it;
          if (buyOrder.userId == order.userId) {
            // We can't trade with ourselves!
            ++it;
            continue;
          }
          const auto saleQuantity = std::min(order.quantity, buyOrder.quantity);
          {
            std::stringstream logStr;
            logStr << "T, " << buyOrder.userId << ", " << buyOrder.userOrderId << ", "
                   << order.userId << ", " << order.userOrderId << ", " << salePrice << ", "
                   << saleQuantity;
            logVec.push_back(logStr.str());
          }
          buyLimit.totalQuantity -= saleQuantity;
          order.quantity -= saleQuantity;
          if (saleQuantity == buyOrder.quantity) {
            orderIdMap.erase(buyOrder.userOrderId);
            buyList.erase(std::next(it).base());
          }
        }
        updateTOB = true;

        // We have taken all of the TOB on buy side. Update.
        if (it == buyList.rend()) {
          UpdateTOBBuySide();
        }
      } else {
        break;
      }
    }

    if (updateTOB) {
      const auto isEliminated = (TOBBuySide_ == buySide_.rend());
      std::string TOBPrice    = (isEliminated) ? "-" : std::to_string(TOBBuySide_->first);
      std::string TOBQuantity =
          (isEliminated) ? "-" : std::to_string(TOBBuySide_->second.totalQuantity);
      std::stringstream logStr;
      logStr << "B, B, " << TOBPrice << ", " << TOBQuantity;
      logVec.push_back(logStr.str());
    }

    // Return if we have nothing left to do with this order, or if it is a market order.
    if (order.quantity <= 0 || order.price == 0) {
      return logVec;
    }

    // MAKE THIS O(1) FOR EVERY ENTRY AFTER THE FIRST O(log n) ENTRY.
    const auto inPriceMap = priceMap_.contains(order.price);
    auto& limit           = (inPriceMap) ? *priceMap_[order.price] : sellSide_[order.price];
    if (!inPriceMap) {
      priceMap_[order.price] = &limit;
    }
    auto& list = limit.list;

    limit.totalQuantity += order.quantity;
    UpdateTOBSellSide();
    if (order.price == TOBSellSide_->first) {
      std::stringstream logStr;
      logStr << "B, S, " << order.price << ", " << TOBSellSide_->second.totalQuantity;
      logVec.push_back(logStr.str());
    }
    const auto userOrderId = order.userOrderId;
    list.push_back(std::move(order));
    orderIdMap.insert_or_assign(userOrderId, OrderIt{&list, std::prev(list.end())});
    return logVec;
  }
};

class OrderBooks {
public:
  std::optional<std::vector<std::string>> HandleOrder(Order order) {
    switch (order.orderType) {
      case (Order::OrderType::BUY):
        maxOrderIdMap_[order.userId] = order.userOrderId;
        return orderBooks_[order.symbol].BuyOrder(std::move(order), orderIdMap_);
        break;
      case (Order::OrderType::SELL):
        maxOrderIdMap_[order.userId] = order.userOrderId;
        return orderBooks_[order.symbol].SellOrder(std::move(order), orderIdMap_);
        break;
      case (Order::OrderType::CANCEL):
        return CancelOrder(std::move(order));
        break;
      case (Order::OrderType::FLUSH):
        orderBooks_.clear();
        orderIdMap_.clear();
        maxOrderIdMap_.clear();
        return std::nullopt;
        break;
      default:
        throw std::runtime_error("Invalid Order::OrderType in OrderBooks::HandleOrder");
    }
  }

  void Reset() {
    orderBooks_.clear();
    orderIdMap_.clear();
    maxOrderIdMap_.clear();
  }

private:
  std::vector<std::string> CancelOrder(Order order) {
    const auto maxUserOrderId = ++maxOrderIdMap_[order.userId];
    std::vector<std::string> logVec;
    {
      std::stringstream logStrC;
      logStrC << "C, " << order.userId << ", " << order.userOrderId << ", " << maxUserOrderId;
      logVec.push_back(logStrC.str());
      std::stringstream logStrA;
      logStrA << "A, " << order.userId << ", " << maxUserOrderId;
      logVec.push_back(logStrA.str());
    }
    if (!orderIdMap_.contains(order.userOrderId)) {
      return logVec;
    }
    // O(1)
    auto& orderIt = orderIdMap_.at(order.userOrderId);
    auto _        = orderIt.listPtr->erase(orderIt.it);
    orderIdMap_.erase(order.userOrderId);
    return logVec;
  }

  std::unordered_map<Symbol_type, OrderBook> orderBooks_;
  OrderBook::OrderMap_type orderIdMap_;
  std::unordered_map<UserId_type, int>
      maxOrderIdMap_;  // Because cancel operations don't possess an order Id.
};

#endif  // #ifndef ORDER_BOOK_H
