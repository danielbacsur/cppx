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
  JSON() : type(Class::Null), value(nullptr) {}

  JSON(Null) : type(Class::Null), value(nullptr) {}

  JSON(Boolean b) : type(Class::Boolean), value(b) {}

  JSON(Integral i) : type(Class::Integral), value(i) {}

  JSON(Floating d) : type(Class::Floating), value(d) {}

  JSON(const String& s) : type(Class::String), value(s) {}

  JSON(String&& s) : type(Class::String), value(std::move(s)) {}

  JSON(const char* s) : type(Class::String), value(String(s)) {}

  JSON(const Array& arr) : type(Class::Array), value(arr) {}

  JSON(Array&& arr) : type(Class::Array), value(std::move(arr)) {}

  JSON(const Object& obj) : type(Class::Object), value(obj) {}

  JSON(Object&& obj) : type(Class::Object), value(std::move(obj)) {}

  JSON(std::initializer_list<JSON> init) {
    if (init.size() % 2 != 0) {
      throw std::invalid_argument("Initializer list must contain an even number of elements (key-value pairs).");
    }

    Object obj;
    auto it = init.begin();
    while (it != init.end()) {
      if (it->type != Class::String) {
        throw std::invalid_argument("Keys must be strings.");
      }
      String key = std::get<String>(it->value);
      ++it;
      if (it == init.end()) {
        throw std::invalid_argument("Missing value for key: " + key);
      }
      JSON val = *it;
      obj.emplace_back(std::make_pair(std::move(key), std::move(val)));
      ++it;
    }

    type = Class::Object;
    value = std::move(obj);
  }

  std::string stringify() const {
    std::ostringstream oss;
    stringify_helper(oss);
    return oss.str();
  }

  static JSON parse(const std::string& s) {
    size_t pos = 0;
    return parse_helper(s, pos);
  }

  friend std::ostream& operator<<(std::ostream& os, const JSON& json) {
    if (json.type == Class::String) {
      return os << std::get<String>(json.value);
    } else {
      return os << json.stringify();
    }
  }

  bool operator==(const JSON& rhs) const {
    if (type != rhs.type) {
      return false;
    }

    switch (type) {
      case Class::Null:
        return true;
      case Class::Boolean:
        return std::get<Boolean>(value) == std::get<Boolean>(rhs.value);
      case Class::Integral:
        return std::get<Integral>(value) == std::get<Integral>(rhs.value);
      case Class::Floating:
        return std::get<Floating>(value) == std::get<Floating>(rhs.value);
      case Class::String:
        return std::get<String>(value) == std::get<String>(rhs.value);
      case Class::Array:
        return std::get<Array>(value) == std::get<Array>(rhs.value);
      case Class::Object:
        return std::get<Object>(value) == std::get<Object>(rhs.value);
    }

    return false;
  }

  JSON& operator[](const std::string& key) {
    if (type != Class::Object) {
      type = Class::Object;
      value = Object();
    }
    Object& obj = std::get<Object>(value);
    auto it = std::find_if(obj.begin(), obj.end(), [&](const std::pair<std::string, JSON>& pair) { return pair.first == key; });
    if (it != obj.end()) {
      return it->second;
    } else {
      obj.emplace_back(std::make_pair(key, JSON()));
      return obj.back().second;
    }
  }

  const JSON& operator[](const std::string& key) const {
    if (type != Class::Object) {
      throw std::runtime_error("JSON value is not an object.");
    }
    const Object& obj = std::get<Object>(value);
    auto it = std::find_if(obj.begin(), obj.end(), [&](const std::pair<std::string, JSON>& pair) { return pair.first == key; });
    if (it != obj.end()) {
      return it->second;
    } else {
      throw std::out_of_range("Key not found: " + key);
    }
  }

 private:
  static std::string escapeString(const String& s) {
    std::ostringstream oss;
    for (char c : s) {
      switch (c) {
        case '\"':
          oss << "\\\"";
          break;
        case '\\':
          oss << "\\\\";
          break;
        case '\b':
          oss << "\\b";
          break;
        case '\f':
          oss << "\\f";
          break;
        case '\n':
          oss << "\\n";
          break;
        case '\r':
          oss << "\\r";
          break;
        case '\t':
          oss << "\\t";
          break;
        default:
          if (static_cast<unsigned char>(c) < 0x20) {
            oss << "\\u" << std::hex << std::setw(4) << std::setfill('0') << (int)c;
          } else {
            oss << c;
          }
      }
    }
    return oss.str();
  }

  static std::string codepoint_to_utf8(unsigned int cp) {
    std::string utf8;
    if (cp <= 0x7F) {
      utf8 += static_cast<char>(cp);
    } else if (cp <= 0x7FF) {
      utf8 += static_cast<char>(0xC0 | ((cp >> 6) & 0x1F));
      utf8 += static_cast<char>(0x80 | (cp & 0x3F));
    } else if (cp <= 0xFFFF) {
      utf8 += static_cast<char>(0xE0 | ((cp >> 12) & 0x0F));
      utf8 += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
      utf8 += static_cast<char>(0x80 | (cp & 0x3F));
    } else if (cp <= 0x10FFFF) {
      utf8 += static_cast<char>(0xF0 | ((cp >> 18) & 0x07));
      utf8 += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
      utf8 += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
      utf8 += static_cast<char>(0x80 | (cp & 0x3F));
    } else {
      throw std::runtime_error("Invalid Unicode code point: " + std::to_string(cp));
    }
    return utf8;
  }

  static std::string parse_unicode_escape(const std::string& s, size_t& pos) {
    pos++;
    if (pos + 4 > s.size()) {
      throw std::runtime_error("Incomplete Unicode escape sequence");
    }
    std::string hex = s.substr(pos, 4);
    pos += 4;

    unsigned int code_unit;
    std::istringstream iss(hex);
    iss >> std::hex >> code_unit;
    if (iss.fail()) {
      throw std::runtime_error("Invalid Unicode escape sequence: \\u" + hex);
    }

    if (code_unit >= 0xD800 && code_unit <= 0xDBFF) {
      if (pos + 2 >= s.size() || s[pos] != '\\' || s[pos + 1] != 'u') {
        throw std::runtime_error("Expected low surrogate after high surrogate");
      }
      pos += 2;
      if (pos + 4 > s.size()) {
        throw std::runtime_error("Incomplete Unicode escape sequence for low surrogate");
      }
      std::string low_hex = s.substr(pos, 4);
      pos += 4;
      unsigned int low_code_unit;
      std::istringstream iss_low(low_hex);
      iss_low >> std::hex >> low_code_unit;
      if (iss_low.fail()) {
        throw std::runtime_error("Invalid Unicode escape sequence: \\u" + low_hex);
      }
      if (!(low_code_unit >= 0xDC00 && low_code_unit <= 0xDFFF)) {
        throw std::runtime_error("Invalid low surrogate: \\u" + low_hex);
      }

      unsigned int high_ten = code_unit - 0xD800;
      unsigned int low_ten = low_code_unit - 0xDC00;
      unsigned int combined = 0x10000 + ((high_ten << 10) | low_ten);
      return codepoint_to_utf8(combined);
    } else if (code_unit >= 0xDC00 && code_unit <= 0xDFFF) {
      throw std::runtime_error("Unexpected low surrogate without preceding high surrogate");
    } else {
      return codepoint_to_utf8(code_unit);
    }
  }

  static void skip_whitespace(const std::string& s, size_t& pos) {
    while (pos < s.size() && std::isspace(static_cast<unsigned char>(s[pos]))) {
      ++pos;
    }
  }

  void stringify_helper(std::ostringstream& oss) const {
    switch (type) {
      case Class::Null:
        oss << "null";
        break;
      case Class::Boolean:
        oss << (std::get<Boolean>(value) ? "true" : "false");
        break;
      case Class::Integral:
        oss << std::get<Integral>(value);
        break;
      case Class::Floating: {
        oss << std::get<Floating>(value);
        break;
      }
      case Class::String:
        oss << '"' << escapeString(std::get<String>(value)) << '"';
        break;
      case Class::Array: {
        oss << '[';
        const Array& arr = std::get<Array>(value);
        for (size_t i = 0; i < arr.size(); ++i) {
          arr[i].stringify_helper(oss);
          if (i != arr.size() - 1) oss << ", ";
        }
        oss << ']';
        break;
      }
      case Class::Object: {
        oss << '{';
        const Object& obj = std::get<Object>(value);
        for (size_t i = 0; i < obj.size(); ++i) {
          oss << '"' << escapeString(obj[i].first) << "\": ";
          obj[i].second.stringify_helper(oss);
          if (i != obj.size() - 1) oss << ", ";
        }
        oss << '}';
        break;
      }
    }
  }

  static JSON parse_helper(const std::string& s, size_t& pos) {
    skip_whitespace(s, pos);
    if (pos >= s.size()) {
      throw std::runtime_error("Unexpected end of input");
    }

    char c = s[pos];
    if (c == 'n') {
      return parse_null(s, pos);
    } else if (c == 't' || c == 'f') {
      return parse_boolean(s, pos);
    } else if (c == '\"') {
      return parse_string(s, pos);
    } else if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) {
      return parse_number(s, pos);
    } else if (c == '[') {
      return parse_array(s, pos);
    } else if (c == '{') {
      return parse_object(s, pos);
    } else {
      throw std::runtime_error(std::string("Invalid character at position ") + std::to_string(pos) + ": " + c);
    }
  }

  static JSON parse_null(const std::string& s, size_t& pos) {
    if (s.compare(pos, 4, "null") == 0) {
      pos += 4;
      return JSON();
    } else {
      throw std::runtime_error("Invalid token, expected 'null'");
    }
  }

  static JSON parse_boolean(const std::string& s, size_t& pos) {
    if (s.compare(pos, 4, "true") == 0) {
      pos += 4;
      return JSON(true);
    } else if (s.compare(pos, 5, "false") == 0) {
      pos += 5;
      return JSON(false);
    } else {
      throw std::runtime_error("Invalid token, expected 'true' or 'false'");
    }
  }

  static JSON parse_number(const std::string& s, size_t& pos) {
    size_t start = pos;
    if (s[pos] == '-') pos++;
    while (pos < s.size() && std::isdigit(static_cast<unsigned char>(s[pos]))) pos++;
    bool is_floating = false;
    if (pos < s.size() && s[pos] == '.') {
      is_floating = true;
      pos++;
      while (pos < s.size() && std::isdigit(static_cast<unsigned char>(s[pos]))) pos++;
    }
    if (pos < s.size() && (s[pos] == 'e' || s[pos] == 'E')) {
      is_floating = true;
      pos++;
      if (pos < s.size() && (s[pos] == '+' || s[pos] == '-')) pos++;
      while (pos < s.size() && std::isdigit(static_cast<unsigned char>(s[pos]))) pos++;
    }
    std::string num_str = s.substr(start, pos - start);
    std::istringstream iss(num_str);
    if (is_floating) {
      Floating d;
      iss >> d;
      if (iss.fail()) throw std::runtime_error("Invalid number: " + num_str);
      return JSON(d);
    } else {
      Integral i;
      iss >> i;
      if (iss.fail()) throw std::runtime_error("Invalid number: " + num_str);
      return JSON(i);
    }
  }

  static JSON parse_string(const std::string& s, size_t& pos) {
    if (s[pos] != '\"') throw std::runtime_error("Expected '\"' at the beginning of string");
    pos++;
    std::ostringstream oss;
    while (pos < s.size()) {
      char c = s[pos];
      if (c == '\"') {
        pos++;
        return JSON(oss.str());
      }
      if (c == '\\') {
        pos++;
        if (pos >= s.size()) throw std::runtime_error("Invalid escape sequence at end of string");
        char esc = s[pos];
        switch (esc) {
          case '\"':
            oss << '\"';
            pos++;
            break;
          case '\\':
            oss << '\\';
            pos++;
            break;
          case '/':
            oss << '/';
            pos++;
            break;
          case 'b':
            oss << '\b';
            pos++;
            break;
          case 'f':
            oss << '\f';
            pos++;
            break;
          case 'n':
            oss << '\n';
            pos++;
            break;
          case 'r':
            oss << '\r';
            pos++;
            break;
          case 't':
            oss << '\t';
            pos++;
            break;
          case 'u': {
            pos--;
            std::string utf8 = parse_unicode_escape(s, pos);
            oss << utf8;
            break;
          }
          default:
            throw std::runtime_error(std::string("Invalid escape character: \\") + esc);
        }
      } else {
        oss << c;
        pos++;
      }
    }
    throw std::runtime_error("Unterminated string");
  }

  static JSON parse_array(const std::string& s, size_t& pos) {
    if (s[pos] != '[') throw std::runtime_error("Expected '[' at beginning of array");
    pos++;
    skip_whitespace(s, pos);
    Array arr;
    if (pos < s.size() && s[pos] == ']') {
      pos++;
      return JSON(arr);
    }
    while (pos < s.size()) {
      JSON value = parse_helper(s, pos);
      arr.emplace_back(std::move(value));
      skip_whitespace(s, pos);
      if (pos >= s.size()) throw std::runtime_error("Unterminated array");
      if (s[pos] == ',') {
        pos++;
        skip_whitespace(s, pos);
      } else if (s[pos] == ']') {
        pos++;
        break;
      } else {
        throw std::runtime_error("Expected ',' or ']' in array");
      }
    }
    return JSON(arr);
  }

  static JSON parse_object(const std::string& s, size_t& pos) {
    if (s[pos] != '{') throw std::runtime_error("Expected '{' at beginning of object");
    pos++;
    skip_whitespace(s, pos);
    Object obj;
    if (pos < s.size() && s[pos] == '}') {
      pos++;
      return JSON(obj);
    }
    while (pos < s.size()) {
      skip_whitespace(s, pos);
      if (s[pos] != '\"') throw std::runtime_error("Expected '\"' at beginning of object key");
      JSON key = parse_string(s, pos);
      skip_whitespace(s, pos);
      if (pos >= s.size() || s[pos] != ':') throw std::runtime_error("Expected ':' after key in object");
      pos++;
      skip_whitespace(s, pos);
      JSON value = parse_helper(s, pos);
      obj.emplace_back(std::make_pair(std::get<String>(key.value), std::move(value)));
      skip_whitespace(s, pos);
      if (pos >= s.size()) throw std::runtime_error("Unterminated object");
      if (s[pos] == ',') {
        pos++;
        skip_whitespace(s, pos);
      } else if (s[pos] == '}') {
        pos++;
        break;
      } else {
        throw std::runtime_error("Expected ',' or '}' in object");
      }
    }
    return JSON(obj);
  }
};

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
    json["mutations"]["array"] = std::vector<JSON>{nullptr, true, 42, 3.14, "Hello, world!"};
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
