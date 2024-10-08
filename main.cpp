#include <chrono>
#include <iostream>
#include <random>
#include <thread>

int main() {
  while (true) {
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist(1000, 9999);
    std::cout << dist(rng) << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
}