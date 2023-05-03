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
#include <string>

using namespace std::chrono_literals;

static std::mutex mtx;

template <typename... Args>
void INFO(Args&&... args) {
// let compiler optimize call without side effects
#ifdef LOG
    std::lock_guard<std::mutex> lock(mtx);
    (std::cout << "info: " << ... << std::forward<Args>(args)) << std::endl;
#endif
}
