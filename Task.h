//
// Created by Geternitier on 2025/4/10.
//

#pragma once

#include <queue>
#include <functional>

class Task {
public:
    int priority;
    std::function<void()> task;

    template<typename Func, typename ...Args>
    Task(int priority, Func func, Args ...args): priority(priority) {
        this->task = std::bind(func, std::forward<Args>(args)...);
    }
    bool operator<(const Task& rhs) const {  return priority < rhs.priority; }
};

class TaskQueue {
    std::priority_queue<Task> queue;
    int maxSize;
public:
    explicit TaskQueue(int size): maxSize(size) {}

    template<typename Func, typename ...Args>
    bool emplace(int priority, Func task, Args ...args) {
        if (queue.size() >= maxSize) return false;
        queue.emplace(priority, task, args...);
        return true;
    }

    std::function<void()> pop() {
        std::function<void()> res = queue.top().task;
        queue.pop();
        return std::move(res);
    }

    bool empty() {
        return queue.empty();
    }

    int size() { return static_cast<int>(queue.size()); }
};