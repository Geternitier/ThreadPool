//
// Created by Geternitier on 2025/4/9.
//
#pragma once

#include <iostream>
#include <vector>
#include <list>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <future>
#include <functional>
#include <chrono>

#include "Task.h"

class DynamicThreadPool {
public:
    DynamicThreadPool(int coreSize, int maxSize, int tasksSize, std::chrono::seconds freeTime):
        coreSize(coreSize), maxSize(maxSize), tasksSize(tasksSize), tasks(tasksSize), freeTime(freeTime){
        start();
    }
    ~DynamicThreadPool() { end(); }

    template<typename Func, typename ...Args>
    bool add(int priority, Func func, Args ...args) {
        bool isSuccess = false;
        {
            std::unique_lock<std::mutex> ulock(tasksMutex);
            if (tasks.size() < tasksSize) {
                tasks.emplace(priority, func, args...);
                isSuccess = true;
            }
        }
        poolSchedule();
        if (isSuccess) tasksCV.notify_one();
        return isSuccess;
    }

private:
    struct TempThread {
        std::thread thread;
        std::future<bool> isEnd;
    };
    std::vector<std::thread> coreThreads;
    std::vector<TempThread> tempThreads;
    std::mutex tempMutex;
    TaskQueue tasks;
    std::condition_variable tasksCV;
    std::mutex tasksMutex;
    bool isEnd = false;
    int coreSize;
    int maxSize;
    std::atomic<int> curSize = 0;
    int tasksSize;
    std::chrono::seconds freeTime;

    void start() { for (int i = 0; i < coreSize; ++i) startCore(); }

    void end() {
        {
            std::unique_lock<std::mutex> ulock(tasksMutex);
            isEnd = true;
        }
        tasksCV.notify_all();
        for (std::thread& thread: coreThreads) thread.join();
        for (TempThread& thread: tempThreads) thread.thread.join();
        poolCollect();
    }

    void startCore() {
        coreThreads.emplace_back([this]{
            while (true) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> ulock(tasksMutex);
                    tasksCV.wait(ulock, [this] {
                        return isEnd || !tasks.empty();
                    });
                    if (isEnd && tasks.empty()) break;
                    task = std::move(tasks.pop());
                }
                ++curSize;
                task();
                --curSize;
            }
        });
    }

    void startTemp() {
        if (curSize >= maxSize) return;
        {
            std::unique_lock<std::mutex> ulock(tasksMutex);
            if (tasks.size() + curSize <= coreSize) return;
        }

        std::unique_lock<std::mutex> ulock(tempMutex);
        if (tempThreads.size() >= maxSize - coreSize) return;

        std::promise<bool> p;
        auto f = p.get_future();
        std::cout << std::format("Temp Start {}\n", tempThreads.size());
        tempThreads.emplace_back(TempThread{
            std::thread([this, p = std::move(p)] mutable {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> ulock(tasksMutex);
                        auto status = tasksCV.wait_for(ulock, freeTime, [this] {
                            return isEnd || !tasks.empty();
                        });
                        if (!status) {
                            p.set_value(true);
                            return;
                        }
                        if (isEnd && tasks.empty()) break;
                        task = std::move(tasks.pop());
                    }
                    ++curSize;
                    task();
                    --curSize;
                }
            }),
            std::move(f)
        });
    }

    void poolSchedule() {
        poolCollect();
        startTemp();
    }

    void poolCollect() {
        std::unique_lock<std::mutex> ulock(tempMutex);
        auto it = tempThreads.begin();
        while (it != tempThreads.end()) {
            if (it->isEnd.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
                if (it->thread.joinable()) it->thread.join();
                it = tempThreads.erase(it);
                std::cout << std::format("Temp Collected, now {}\n", tempThreads.size());
            } else ++it;
        }
    }

};