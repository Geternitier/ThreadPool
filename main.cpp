#include <iostream>
#include <format>
#include "DynamicThreadPool.h"

void exampleTask(int i) {
    std::cout << std::format("Task: {}\n", i);
}

int main() {
    DynamicThreadPool pool(4, 8, 20, std::chrono::seconds(1));
    for (int i = 0; i < 100; ++i) {
        pool.add(1, exampleTask, i);
    }
}
