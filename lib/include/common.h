#pragma once

#include <mutex>
#include <condition_variable>
#include <vector>
#include <atomic>
#include <functional>
#include <chrono>
#include <random>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <thread>
#include <optional>
#include <memory>
#include <future>
#include <iostream>
#include <sstream>

using namespace std::chrono_literals;

inline void INFO(const std::string& str = "") {
    auto myid = std::this_thread::get_id();
    std::stringstream ss;
    ss << myid;
    std::string info = "info: threadID = " + ss.str() + "; " + str + "\n";
    std::cout << info;
}
