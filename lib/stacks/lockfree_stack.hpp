#pragma once
#include <lib/common/task.hpp>

// An example of completely wrong LF stack with ABA problem

template <typename T>
class lfStack {
private:
    struct Node {
        std::atomic<Node*> next;
        T data;

        Node(const T& data): data(data) {
        }
    };

    std::atomic<Node*> top_ = nullptr;

public:
    void push(const T& data) {
        Node* newTop = new Node(data);
        newTop->next = top_;
        while (!top_.compare_exchange_strong(newTop->next, newTop)) {
            // repeat until success
        }
    }

    bool pop(T& data) {
        Node* curTop = top_.load();
        while (true) {
            if (curTop == nullptr) { return false; }
            if (top_.compare_exchange_strong(curTop, curTop->next.load())) {
                data = curTop->data;
                // delete curTop;
                return true;
            }
        }
    }
};

