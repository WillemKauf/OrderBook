#ifndef SCENARIO_H
#define SCENARIO_H

/////////////////
/// std
/////////////////
#include <string>
#include <vector>

/////////////////
/// pybind11
/////////////////
#include <pybind11/embed.h>

/////////////////
/// local
/////////////////
#include "config.h"
#include "order_book.h"

struct Scenario {
  std::vector<Order> orders;
};

namespace py = pybind11;

using RawScenario_type    = std::vector<std::vector<std::vector<std::string>>>;
using ExpectedOutput_type = std::unordered_map<int, std::vector<std::string>>;

inline RawScenario_type ReadScenarios(const std::string& filePath) {
  py::scoped_interpreter guard{};
  py::module::import("sys").attr("path").attr("append")(ROOT_DIR + "/scripts/");
  py::module_ mod      = py::module_::import("read_scenarios");
  py::object result    = mod.attr("read_scenarios")(filePath);
  py::list pyScenarios = py::list(result);
  std::vector<std::vector<std::vector<std::string>>> rawScenarios;

  for (const auto& scenario : pyScenarios) {
    std::vector<std::vector<std::string>> currScenario;
    for (const auto& line : scenario) {
      py::list pyLine = py::cast<py::list>(line);
      std::vector<std::string> currLine;
      for (const auto& item : line) {
        std::string value = py::cast<std::string>(item);
        currLine.push_back(value);
      }
      currScenario.push_back(currLine);
    }
    rawScenarios.push_back(currScenario);
  }
  return rawScenarios;
}

inline std::map<int, Scenario> GetScenarios(const std::string& filePath) {
  RawScenario_type rawScenarios = ReadScenarios(filePath);
  std::map<int, Scenario> scenarios;
  for (int id = 1; const auto& rawScenario : rawScenarios) {
    Scenario scenario;
    for (const auto& list : rawScenario) {
      const auto& orderChar = list[0];
      Order order;
      if (orderChar == "N") {
        const auto& userId         = list[1];
        const auto& symbol         = list[2];
        const auto& price          = list[3];
        const auto& quantity       = list[4];
        const auto& side           = list[5];
        const auto& userOrderId    = list[6];
        Order::OrderType orderType = (side == "B") ? Order::OrderType::BUY : Order::OrderType::SELL;
        order.userId               = std::stoi(userId);
        order.userOrderId          = std::stoi(userOrderId);
        order.orderType            = orderType;
        order.symbol               = symbol;
        order.price                = std::stoi(price);
        order.quantity             = std::stoi(quantity);
      } else if (orderChar == "C") {
        const auto& userId      = list[1];
        const auto& userOrderId = list[2];
        order.userId            = std::stoi(userId);
        order.userOrderId       = std::stoi(userOrderId);
        order.orderType         = Order::OrderType::CANCEL;
      } else if (orderChar == "F") {
        order.orderType = Order::OrderType::FLUSH;
      }
      scenario.orders.push_back(std::move(order));
    }
    scenarios.insert_or_assign(id++, std::move(scenario));
  }

  return scenarios;
}

inline ExpectedOutput_type GetExpectedOutput(const std::string& filePath) {
  RawScenario_type rawScenarios = ReadScenarios(filePath);
  ExpectedOutput_type expectedOutputs;
  for (int id = 1; const auto& rawScenario : rawScenarios) {
    std::vector<std::string> expectedOutput;
    for (const auto& vec : rawScenario) {
      std::string line = {};
      for (const auto& str : vec) {
        line += str + ", ";
      }
      line.pop_back();
      line.pop_back();
      expectedOutput.push_back(line);
    }
    expectedOutputs.insert_or_assign(id++, expectedOutput);
  }

  return expectedOutputs;
}

#endif  // #ifndef SCENARIO_H
