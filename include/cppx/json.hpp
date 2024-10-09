#pragma once

#include <algorithm>
#include <cctype>
#include <chrono>
#include <initializer_list>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <variant>
#include <vector>

class JSON {
 public:
  enum class Class { Null, Boolean, Integral, Floating, String, Array, Object };

 private:
  Class type;

  using Null = std::nullptr_t;
  using Boolean = bool;
  using Integral = int;
  using Floating = double;
  using String = std::string;
  using Array = std::vector<JSON>;
  using Object = std::vector<std::pair<std::string, JSON>>;

  std::variant<Null, Boolean, Integral, Floating, String, Array, Object> value;

 public:
  JSON();
  JSON(Null);
  JSON(Boolean);
  JSON(Integral);
  JSON(Floating);
  JSON(const String&);
  JSON(String&&);
  JSON(const char*);
  JSON(const Array&);
  JSON(Array&&);
  JSON(const Object&);
  JSON(Object&&);
  JSON(std::initializer_list<JSON> init);

  std::string stringify() const;
  static JSON parse(const std::string& s);

  friend std::ostream& operator<<(std::ostream& os, const JSON& json);
  bool operator==(const JSON& rhs) const;
  JSON& operator[](const std::string& key);
  const JSON& operator[](const std::string& key) const;

 private:
  static std::string escapeString(const String& s);
  static std::string codepoint_to_utf8(unsigned int cp);
  static std::string parse_unicode_escape(const std::string& s, size_t& pos);
  static void skip_whitespace(const std::string& s, size_t& pos);
  void stringify_helper(std::ostringstream& oss) const;
  static JSON parse_helper(const std::string& s, size_t& pos);
  static JSON parse_null(const std::string& s, size_t& pos);
  static JSON parse_boolean(const std::string& s, size_t& pos);
  static JSON parse_number(const std::string& s, size_t& pos);
  static JSON parse_string(const std::string& s, size_t& pos);
  static JSON parse_array(const std::string& s, size_t& pos);
  static JSON parse_object(const std::string& s, size_t& pos);
};
