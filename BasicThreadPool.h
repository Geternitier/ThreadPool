//
// Created by Geternitier on 2025/4/9.
//

#pragma once

#include <iostream>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>

class BasicThreadPool {
public:
    explicit BasicThreadPool(int threadNum) { start(threadNum); }
    ~BasicThreadPool() { stop(); }

    void add(const std::function<void()>& task) {
        {
            std::unique_lock<std::mutex> ulock(tasksMutex);
            tasks.emplace(task);
        }
        tasksCV.notify_one();
    }

private:
    std::vector<std::thread> threads;
    std::queue<std::function<void()>> tasks;
    std::condition_variable tasksCV;
    std::mutex tasksMutex;
    bool stopping = false;

    void start(int threadNum) {
        for (int i = 0; i < threadNum; ++i) {
            threads.emplace_back([this]{
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> ulock(tasksMutex);
                        tasksCV.wait(ulock, [this] {
                            return stopping || !tasks.empty();
                        });
                        if (stopping && tasks.empty()) break;
                        task = std::move(tasks.front());
                        tasks.pop();
                    }
                    task();
                }
            });
        }
    }

    void stop() {
        {
            std::unique_lock<std::mutex> ulock(tasksMutex);
            stopping = true;
        }
        tasksCV.notify_all();
        for (std::thread& thread: threads) thread.join();
    }

};


