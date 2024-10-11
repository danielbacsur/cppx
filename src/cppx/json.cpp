#include "cppx/json.hpp"

JSON::JSON() : type_(Type::Null), value_(std::in_place_type<std::monostate>) {}

JSON::JSON(Null) : type_(Type::Null), value_(std::in_place_type<std::monostate>) {}

JSON::JSON(Boolean value) : type_(Type::Boolean), value_(std::in_place_type<Boolean>, value) {}

JSON::JSON(Integer value) : type_(Type::Integer), value_(std::in_place_type<Integer>, value) {}

JSON::JSON(Floating value) : type_(Type::Floating), value_(std::in_place_type<Floating>, value) {}

JSON::JSON(const String& value) : type_(Type::String), value_(std::in_place_type<String>, value) {}

JSON::JSON(String&& value) : type_(Type::String), value_(std::in_place_type<String>, std::move(value)) {}

JSON::JSON(const char* value) : type_(Type::String), value_(std::in_place_type<String>, value) {}

JSON::JSON(const Array& value) : type_(Type::Array), value_(std::in_place_type<Array>, value) {}

JSON::JSON(Array&& value) : type_(Type::Array), value_(std::in_place_type<Array>, std::move(value)) {}

JSON::JSON(const Object& value) : type_(Type::Object), value_(std::in_place_type<Object>, value) {}

JSON::JSON(Object&& value) : type_(Type::Object), value_(std::in_place_type<Object>, std::move(value)) {}

JSON::JSON(std::initializer_list<JSON> init) {
  if (init.size() % 2 != 0) {
    throw std::invalid_argument("Initializer list must contain an even number of elements (key-value pairs).");
  }
  
  Object obj;
  auto it = init.begin();
  while (it != init.end()) {
    if (it->type_ != Type::String) {
      throw std::invalid_argument("Keys must be strings.");
    }
    String key = std::get<String>(it->value_);
    ++it;
    if (it == init.end()) {
      throw std::invalid_argument("Missing value for key: " + key);
    }
    JSON val = *it;
    obj.emplace_back(std::make_pair(std::move(key), std::move(val)));
    ++it;
  }
  type_ = Type::Object;
  value_.emplace<Object>(std::move(obj));
}

JSON::operator Null() const {
  if (type_ != Type::Null) {
    throw std::runtime_error("JSON value is not null.");
  }
  return Null{};
}

JSON::operator Boolean() const {
  if (type_ != Type::Boolean) {
    throw std::runtime_error("JSON value is not a boolean.");
  }
  return std::get<Boolean>(value_);
}

JSON::operator Integer() const {
  if (type_ != Type::Integer) {
    throw std::runtime_error("JSON value is not an integer.");
  }
  return std::get<Integer>(value_);
}

JSON::operator Floating() const {
  if (type_ != Type::Floating) {
    throw std::runtime_error("JSON value is not a floating-point number.");
  }
  return std::get<Floating>(value_);
}

JSON::operator String() const {
  if (type_ != Type::String) {
    throw std::runtime_error("JSON value is not a string.");
  }
  return std::get<String>(value_);
}

JSON::operator Array() const {
  if (type_ != Type::Array) {
    throw std::runtime_error("JSON value is not an array.");
  }
  return std::get<Array>(value_);
}

JSON::operator Object() const {
  if (type_ != Type::Object) {
    throw std::runtime_error("JSON value is not an object.");
  }
  return std::get<Object>(value_);
}

JSON::operator Callable() const {
  if (type_ != Type::Callable) {
    throw std::runtime_error("JSON value is not a callable.");
  }
  return std::get<Callable>(value_);
}

JSON& JSON::operator[](const std::string& key) {
  if (type_ != Type::Object) {
    type_ = Type::Object;
    value_.emplace<Object>();
  }
  Object& obj = std::get<Object>(value_);
  for (auto& pair : obj) {
    if (pair.first == key) {
      return pair.second;
    }
  }
  obj.emplace_back(std::make_pair(key, JSON()));
  return obj.back().second;
}

const JSON& JSON::operator[](const std::string& key) const {
  if (type_ != Type::Object) {
    throw std::runtime_error("JSON value is not an object.");
  }
  const Object& obj = std::get<Object>(value_);
  for (const auto& pair : obj) {
    if (pair.first == key) {
      return pair.second;
    }
  }
  throw std::out_of_range("Key not found: " + key);
}

JSON& JSON::operator[](size_t index) {
  if (type_ != Type::Array) {
    throw std::runtime_error("JSON value is not an array.");
  }
  Array& arr = std::get<Array>(value_);
  if (index >= arr.size()) {
    throw std::out_of_range("Index out of range.");
  }
  return arr[index];
}

const JSON& JSON::operator[](size_t index) const {
  if (type_ != Type::Array) {
    throw std::runtime_error("JSON value is not an array.");
  }
  const Array& arr = std::get<Array>(value_);
  if (index >= arr.size()) {
    throw std::out_of_range("Index out of range.");
  }
  return arr[index];
}

std::ostream& operator<<(std::ostream& os, const JSON& json) {
  switch (json.type_) {
    case JSON::Type::Null:
      os << "null";
      break;
    case JSON::Type::Boolean:
      os << (std::get<JSON::Boolean>(json.value_) ? "true" : "false");
      break;
    case JSON::Type::Integer:
      os << std::get<JSON::Integer>(json.value_);
      break;
    case JSON::Type::Floating:
      os << std::get<JSON::Floating>(json.value_);
      break;
    case JSON::Type::String:
      os << '"' << JSON::escapeString(std::get<JSON::String>(json.value_)) << '"';
      break;
    case JSON::Type::Array: {
      os << '[';
      const JSON::Array& arr = std::get<JSON::Array>(json.value_);
      for (size_t i = 0; i < arr.size(); ++i) {
        os << arr[i];
        if (i != arr.size() - 1) os << ", ";
      }
      os << ']';
      break;
    }
    case JSON::Type::Object: {
      os << '{';
      const JSON::Object& obj = std::get<JSON::Object>(json.value_);
      for (size_t i = 0; i < obj.size(); ++i) {
        os << '"' << JSON::escapeString(obj[i].first) << "\": " << obj[i].second;
        if (i != obj.size() - 1) os << ", ";
      }
      os << '}';
      break;
    }
    case JSON::Type::Callable:
      os << "<callable>";
      break;
  }
  return os;
}

bool JSON::operator==(const JSON& rhs) const {
  if (type_ != rhs.type_) {
    return false;
  }
  switch (type_) {
    case Type::Null:
      return true;
    case Type::Boolean:
      return std::get<Boolean>(value_) == std::get<Boolean>(rhs.value_);
    case Type::Integer:
      return std::get<Integer>(value_) == std::get<Integer>(rhs.value_);
    case Type::Floating:
      return std::get<Floating>(value_) == std::get<Floating>(rhs.value_);
    case Type::String:
      return std::get<String>(value_) == std::get<String>(rhs.value_);
    case Type::Array:
      return std::get<Array>(value_) == std::get<Array>(rhs.value_);
    case Type::Object:
      return std::get<Object>(value_) == std::get<Object>(rhs.value_);
    case Type::Callable:
      return false;
  }
  return false;
}

bool JSON::operator!=(const JSON& rhs) const { return !(*this == rhs); }

std::string JSON::stringify() const {
  std::ostringstream oss;
  stringifyHelper(oss);
  return oss.str();
}

void JSON::stringifyHelper(std::ostringstream& oss) const {
  switch (type_) {
    case Type::Null:
      oss << "null";
      break;
    case Type::Boolean:
      oss << (std::get<Boolean>(value_) ? "true" : "false");
      break;
    case Type::Integer:
      oss << std::get<Integer>(value_);
      break;
    case Type::Floating:
      oss << std::get<Floating>(value_);
      break;
    case Type::String:
      oss << '"' << escapeString(std::get<String>(value_)) << '"';
      break;
    case Type::Array: {
      oss << '[';
      const Array& arr = std::get<Array>(value_);
      for (size_t i = 0; i < arr.size(); ++i) {
        arr[i].stringifyHelper(oss);
        if (i != arr.size() - 1) oss << ", ";
      }
      oss << ']';
      break;
    }
    case Type::Object: {
      oss << '{';
      const Object& obj = std::get<Object>(value_);
      for (size_t i = 0; i < obj.size(); ++i) {
        oss << '"' << escapeString(obj[i].first) << "\": ";
        obj[i].second.stringifyHelper(oss);
        if (i != obj.size() - 1) oss << ", ";
      }
      oss << '}';
      break;
    }
    case Type::Callable:
      oss << "<callable>";
      break;
  }
}

JSON JSON::parse(const std::string& s) {
  size_t pos = 0;
  JSON result = parseHelper(s, pos);
  skipWhitespace(s, pos);
  if (pos != s.size()) {
    throw std::runtime_error("Extra characters after parsing JSON.");
  }
  return result;
}

void JSON::skipWhitespace(const std::string& s, size_t& pos) {
  while (pos < s.size() && std::isspace(static_cast<unsigned char>(s[pos]))) {
    ++pos;
  }
}

std::string JSON::escapeString(const String& s) {
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

std::string JSON::codepointToUTF8(unsigned int cp) {
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

std::string JSON::parseUnicodeEscape(const std::string& s, size_t& pos) {
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
    return codepointToUTF8(combined);
  } else if (code_unit >= 0xDC00 && code_unit <= 0xDFFF) {
    throw std::runtime_error("Unexpected low surrogate without preceding high surrogate");
  } else {
    return codepointToUTF8(code_unit);
  }
}

JSON JSON::parseHelper(const std::string& s, size_t& pos) {
  skipWhitespace(s, pos);
  if (pos >= s.size()) {
    throw std::runtime_error("Unexpected end of input");
  }

  char c = s[pos];
  if (c == 'n') {
    return parseNull(s, pos);
  } else if (c == 't' || c == 'f') {
    return parseBoolean(s, pos);
  } else if (c == '\"') {
    return parseString(s, pos);
  } else if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) {
    return parseNumber(s, pos);
  } else if (c == '[') {
    return parseArray(s, pos);
  } else if (c == '{') {
    return parseObject(s, pos);
  } else {
    throw std::runtime_error(std::string("Invalid character at position ") + std::to_string(pos) + ": " + c);
  }
}

JSON JSON::parseNull(const std::string& s, size_t& pos) {
  if (s.compare(pos, 4, "null") == 0) {
    pos += 4;
    return JSON();
  } else {
    throw std::runtime_error("Invalid token, expected 'null'");
  }
}

JSON JSON::parseBoolean(const std::string& s, size_t& pos) {
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

JSON JSON::parseNumber(const std::string& s, size_t& pos) {
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
    Integer i;
    iss >> i;
    if (iss.fail()) throw std::runtime_error("Invalid number: " + num_str);
    return JSON(i);
  }
}

JSON JSON::parseString(const std::string& s, size_t& pos) {
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
          std::string utf8 = parseUnicodeEscape(s, pos);
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

JSON JSON::parseArray(const std::string& s, size_t& pos) {
  if (s[pos] != '[') throw std::runtime_error("Expected '[' at beginning of array");
  pos++;
  skipWhitespace(s, pos);
  Array arr;
  if (pos < s.size() && s[pos] == ']') {
    pos++;
    return JSON(arr);
  }
  while (pos < s.size()) {
    JSON value = parseHelper(s, pos);
    arr.emplace_back(std::move(value));
    skipWhitespace(s, pos);
    if (pos >= s.size()) throw std::runtime_error("Unterminated array");
    if (s[pos] == ',') {
      pos++;
      skipWhitespace(s, pos);
    } else if (s[pos] == ']') {
      pos++;
      break;
    } else {
      throw std::runtime_error("Expected ',' or ']' in array");
    }
  }
  return JSON(arr);
}

JSON JSON::parseObject(const std::string& s, size_t& pos) {
  if (s[pos] != '{') throw std::runtime_error("Expected '{' at beginning of object");
  pos++;
  skipWhitespace(s, pos);
  Object obj;
  if (pos < s.size() && s[pos] == '}') {
    pos++;
    return JSON(obj);
  }
  while (pos < s.size()) {
    skipWhitespace(s, pos);
    if (s[pos] != '\"') throw std::runtime_error("Expected '\"' at beginning of object key");
    JSON key = parseString(s, pos);
    skipWhitespace(s, pos);
    if (pos >= s.size() || s[pos] != ':') throw std::runtime_error("Expected ':' after key in object");
    pos++;
    skipWhitespace(s, pos);
    JSON value = parseHelper(s, pos);
    obj.emplace_back(std::make_pair(std::get<String>(key.value_), std::move(value)));
    skipWhitespace(s, pos);
    if (pos >= s.size()) throw std::runtime_error("Unterminated object");
    if (s[pos] == ',') {
      pos++;
      skipWhitespace(s, pos);
    } else if (s[pos] == '}') {
      pos++;
      break;
    } else {
      throw std::runtime_error("Expected ',' or '}' in object");
    }
  }
  return JSON(obj);
}

JSON::Type JSON::type() const { return type_; }
