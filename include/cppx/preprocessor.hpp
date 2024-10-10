#pragma once

#include <algorithm>
#include <cctype>
#include <iostream>
#include <regex>
#include <sstream>
#include <stack>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class Preprocessor {
 public:
  static std::string Process(const std::string& input);

 private:
  struct DOMNode {
    std::string type;
    std::string tagName;
    std::unordered_map<std::string, std::string> attributes;
    std::vector<DOMNode> children;
    std::string textContent;
  };

  static std::string AddHeader(const std::string& script);
  static std::vector<std::string> ExtractValidHtmlBlocks(const std::string& script);
  static DOMNode ParseHTML(const std::string& html, size_t& pos);
  static std::string GenerateJSON(const DOMNode& node, int indent = 0);
  static std::string CorrectIndentation(const std::string& code);
  static std::string Trim(const std::string& str);
  static std::unordered_map<std::string, std::string> ParseAttributes(const std::string& attrString);
  static const std::unordered_set<std::string> htmlTags;
  static const std::string header;
};
