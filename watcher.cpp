#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <thread>

std::atomic<bool> running(true);
const std::string filename = "main.cpp";
const std::string output_directory = ".cppx";
const std::string output_binary = output_directory + "/main-cppx";

int main() {
  std::signal(SIGINT, [](int signum) {
    running = false;
    std::system(("pkill -f " + output_binary).c_str());
  });
  
  if (!std::filesystem::exists(output_directory)) {
    std::filesystem::create_directory(output_directory);
  }

  std::cout << "Watcher started. Press Ctrl+C to exit.\n";

  std::time_t last_write_time;

  while (running) {
    auto current_write_time = std::filesystem::last_write_time(filename).time_since_epoch().count();

    if (current_write_time != last_write_time) {
      std::system(("pkill -f " + output_binary).c_str());
      std::system(("g++ " + filename + " -o " + output_binary + " -std=c++20").c_str());
      std::system((output_binary + " &").c_str());
      last_write_time = current_write_time;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }

  std::cout << "Watcher stopped." << std::endl;

  return 0;
}
