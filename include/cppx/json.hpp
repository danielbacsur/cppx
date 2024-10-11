#pragma once

#include <algorithm>
#include <cctype>
#include <functional>
#include <initializer_list>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
#include <vector>

class JSON {
 public:
  using Null = std::monostate;
  using Boolean = bool;
  using Integer = int;
  using Floating = double;
  using String = std::string;
  using Array = std::vector<JSON>;
  using Object = std::vector<std::pair<std::string, JSON>>;
  using Callable = std::function<void()>;

  enum class Type { Null, Boolean, Integer, Floating, String, Array, Object, Callable };

  JSON();
  JSON(Null);
  JSON(Boolean value);
  JSON(Integer value);
  JSON(Floating value);
  JSON(const String& value);
  JSON(String&& value);
  JSON(const char* value);
  JSON(const Array& value);
  JSON(Array&& value);
  JSON(const Object& value);
  JSON(Object&& value);

  template <typename F, typename = std::enable_if_t<std::is_invocable_r_v<void, F>>>
  JSON(F&& func) : type_(Type::Callable), value_(Callable(std::forward<F>(func))) {}

  JSON(std::initializer_list<JSON> init);

  explicit operator Null() const;
  explicit operator Boolean() const;
  explicit operator Integer() const;
  explicit operator Floating() const;
  explicit operator String() const;
  explicit operator Array() const;
  explicit operator Object() const;
  explicit operator Callable() const;

  JSON& operator[](const std::string& key);
  const JSON& operator[](const std::string& key) const;
  JSON& operator[](size_t index);
  const JSON& operator[](size_t index) const;

  friend std::ostream& operator<<(std::ostream& os, const JSON& json);

  bool operator==(const JSON& rhs) const;
  bool operator!=(const JSON& rhs) const;

  std::string stringify() const;
  static JSON parse(const std::string& s);

 private:
  Type type_;
  std::variant<Null, Boolean, Integer, Floating, String, Array, Object, Callable> value_;

  static std::string escapeString(const String& s);
  static std::string codepointToUTF8(unsigned int cp);
  static std::string parseUnicodeEscape(const std::string& s, size_t& pos);
  static void skipWhitespace(const std::string& s, size_t& pos);

  void stringifyHelper(std::ostringstream& oss) const;

  static JSON parseHelper(const std::string& s, size_t& pos);
  static JSON parseNull(const std::string& s, size_t& pos);
  static JSON parseBoolean(const std::string& s, size_t& pos);
  static JSON parseNumber(const std::string& s, size_t& pos);
  static JSON parseString(const std::string& s, size_t& pos);
  static JSON parseArray(const std::string& s, size_t& pos);
  static JSON parseObject(const std::string& s, size_t& pos);
};
