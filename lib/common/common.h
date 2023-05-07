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
#include <optional>
#include <cassert>
#include <queue>
#include <tuple>

using namespace std::chrono_literals;

static std::mutex mtx;

template <typename T>
std::ostream& operator<<(std::ostream& out, const std::vector<T>& vec) {
    for (size_t i = 0; i < vec.size(); i++) {
        out << vec[i] << ' ';
    }
    return out;
}

template <typename... Args>
inline void INFO(Args&&... args) {
// let compiler optimize call without side effects
#ifdef LOG
    std::lock_guard<std::mutex> lock(mtx);
    (std::cout << "info: " << ... << std::forward<Args>(args)) << std::endl;
#endif
}

template <typename... Args>
inline void REQUIRE(bool cond, Args&&... args) {
// let compiler optimize call without side effects
    if (!cond) {
        std::lock_guard<std::mutex> lock(mtx);
        (std::cout << "require failed: " << ... << std::forward<Args>(args)) << std::endl;
        std::abort();
    }
}