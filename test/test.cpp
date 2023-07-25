/////////////////
/// std
/////////////////
#include <asm/unistd.h>
#include <linux/perf_event.h>
#include <sys/ioctl.h>
#include <unistd.h>

/////////////////
/// gtest
/////////////////
#include <gtest/gtest.h>

/////////////////
/// local
/////////////////
#include "client_manager.h"
#include "scenario.h"
#include "server_manager.h"

static const auto scenarios       = GetScenarios(ROOT_DIR + "/data/inputfile.csv");
static const auto expectedOutputs = GetExpectedOutput(ROOT_DIR + "/data/outputfile.csv");

struct PerfTool {
  PerfTool() {
    const auto peConfigsSize = peConfigs.size();
    peMap.resize(peConfigsSize);
    fdMap.resize(peConfigsSize);
    perfMap.resize(peConfigsSize);
    for (int i = 0; const auto& config : peConfigs) {
      perf_event_attr pe;
      pe.type           = PERF_TYPE_HARDWARE;
      pe.size           = sizeof(pe);
      pe.config         = config;
      pe.disabled       = 1;
      pe.exclude_hv     = 1;
      pe.exclude_kernel = 1;
      peMap[i]          = std::move(pe);
      fdMap[i]          = syscall(__NR_perf_event_open, &peMap[i], 0, -1, -1, 0);
      ++i;
    }
  }

  ~PerfTool() {
    for (auto& fd : fdMap) {
      close(fd);
    }
  }

  const std::vector<int> peConfigs = {
      PERF_COUNT_HW_CACHE_MISSES, PERF_COUNT_HW_CACHE_REFERENCES, PERF_COUNT_HW_BRANCH_MISSES,
      PERF_COUNT_HW_BRANCH_INSTRUCTIONS, PERF_COUNT_HW_INSTRUCTIONS};
  std::vector<perf_event_attr> peMap;
  std::vector<int> fdMap;
  std::vector<unsigned long long> perfMap;
};

static PerfTool perfTool;

void TestScenario(int i) {
  //for (std::size_t i = 0; i < perfTool.peConfigs.size(); ++i) {
  //  ioctl(perfTool.fdMap[i], PERF_EVENT_IOC_RESET, 0);
  //  ioctl(perfTool.fdMap[i], PERF_EVENT_IOC_ENABLE, 0);
  //}
  //....
  //Mostly useless now, since I moved the bulk of the processing to the server process.
  //auto& perfMap = perfTool.perfMap;
  //for (std::size_t i = 0; i < perfTool.peConfigs.size(); ++i) {
  //  ioctl(perfTool.fdMap[i], PERF_EVENT_IOC_DISABLE, 0);
  //  read(perfTool.fdMap[i], &perfMap[i], sizeof(perfMap[i]));
  //}

  const std::string inputFileStr = ROOT_DIR + "/logs/" + std::to_string(i) + ".log";
  std::ifstream inputFile(inputFileStr);

  if(!inputFile.is_open()){
    throw std::runtime_error("Couldn't read from inputFile in TestScenario(). Make sure you run the main executables first.");
  }

  std::vector<std::string> serverLog;
  for (std::string line; std::getline(inputFile, line);) {
    serverLog.push_back(line);
  }

  const auto& expectedLog = expectedOutputs.at(i);
  EXPECT_EQ(serverLog.size(), expectedLog.size());

  std::cout << "======================\n";
  std::cout << "SCENARIO " << i << '\n';
  for (std::size_t i = 0; i < serverLog.size(); ++i) {
    std::cout << serverLog[i] << '\n';
    EXPECT_EQ(serverLog[i], expectedLog[i]);
  }


  //std::cout << "Cache misses/references: " << perfMap[0] << "/" << perfMap[1] << " "
  //          << static_cast<float>(perfMap[0]) / static_cast<float>(perfMap[1]) * 100 << "%\n"
  //          << "Branch misses/instructions: " << perfMap[2] << "/" << perfMap[3] << " "
  //          << static_cast<float>(perfMap[2]) / static_cast<float>(perfMap[3]) * 100 << "%\n"
  //          << "HW instructions: " << perfMap[4] << '\n';
}

TEST(OrderBook, Scenario1) { TestScenario(1); }
TEST(OrderBook, Scenario2) { TestScenario(2); }
TEST(OrderBook, Scenario3) { TestScenario(3); }
TEST(OrderBook, Scenario4) { TestScenario(4); }
TEST(OrderBook, Scenario5) { TestScenario(5); }
TEST(OrderBook, Scenario6) { TestScenario(6); }
TEST(OrderBook, Scenario7) { TestScenario(7); }
TEST(OrderBook, Scenario8) { TestScenario(8); }
TEST(OrderBook, Scenario9) { TestScenario(9); }
TEST(OrderBook, Scenario10) { TestScenario(10); }
TEST(OrderBook, Scenario11) { TestScenario(11); }
TEST(OrderBook, Scenario12) { TestScenario(12); }
TEST(OrderBook, Scenario13) { TestScenario(13); }
TEST(OrderBook, Scenario14) { TestScenario(14); }
TEST(OrderBook, Scenario15) { TestScenario(15); }
TEST(OrderBook, Scenario16) { TestScenario(16); }
