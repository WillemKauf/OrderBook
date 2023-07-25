/////////////////
/// std
/////////////////
#include <chrono>
#include <iostream>
#include <thread>

/////////////////
/// local
/////////////////
#include "client_manager.h"
#include "config.h"
#include "scenario.h"

int main() {
  const auto scenarios = GetScenarios(ROOT_DIR + "/data/inputfile.csv");
  auto& clientManager  = ClientManager::GetClientManager();
  for (const auto& [scenarioId, scenario] : scenarios) {
    for (const auto& order : scenario.orders) {
      clientManager.SubmitOrder(order);
    }
  }
}
