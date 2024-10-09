#include <chrono>
#include <iostream>
#include <random>
#include <string>
#include <thread>

#include "cppx/json.hpp"

int main() {
  std::mt19937 rng(std::random_device{}());
  std::uniform_int_distribution<int> dist(1000, 9999);

  while (true) {
    JSON json = {
      "html", {
        "children", {
          "head", {
            "children", {
              "title", nullptr
            }
          },
          "body", {
            "children", {
              "h1", {
                "children", "Welcome to the landing page!"
              },
              "p", {
                "children", "Your random number is: " + std::to_string(dist(rng))
              },
              "", "Hello, ",
              "strong", {
                "children", "world"
              },
              "", "!",
              "footer", {
                "children", "© 2024 CPPX 🚀"
              }
            }
          }
        }
      }
    };

    json["mutations"]["boolean"] = true;
    json["mutations"]["integral"] = 420;
    json["mutations"]["floating"] = 3.14;
    json["mutations"]["string"] = "Hello, world!";
    json["mutations"]["array"] = std::vector<JSON>{nullptr, true, 420, 3.14, "Hello, world!"};
    json["mutations"]["object"] = JSON{
        "first", 1,
        "second", 2,
        "third", 3
    };

    std::cout << "\033[2J\033[1;1H" << json << std::endl;

    try {
      JSON parsed_json = JSON::parse(json.stringify());
    } catch (const std::exception& e) {
      std::cerr << "Parse error: " << e.what() << std::endl;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  return 0;
}
