#include "cppx/preprocessor.hpp"

const std::unordered_set<std::string> Preprocessor::htmlTags = {
  "html", "head", "body", "title", "meta", "link", "script", "style",
  "h1", "h2", "h3", "h4", "h5", "h6", "p", "span", "div", "br", "hr",
  "ul", "ol", "li", "dl", "dt", "dd",
  "a", "img", "figure", "figcaption",
  "form", "input", "textarea", "button", "select", "option", "label", "fieldset", "legend",
  "table", "thead", "tbody", "tfoot", "tr", "th", "td", "caption", "colgroup", "col",
  "header", "footer", "nav", "main", "article", "section", "aside", "details", "summary",
  "iframe", "audio", "video", "source", "canvas", "svg",
  "strong", "em", "code", "pre", "blockquote", "q", "cite", "abbr",
  "time", "mark", "small", "sub", "sup",
  "dialog", "menu", "progress", "meter",
  "base", "noscript"
};

const std::string Preprocessor::header = (
  "// WARNING: This file has been automatically generated or modified.\n"
  "// Any manual changes may be overwritten in future updates.\n"
  "\n"
  "#include \"cppx/json.hpp\"\n"
  "#include \"cppx/page.hpp\"\n"
);

std::string Preprocessor::Trim(const std::string& str) {
  const std::string whitespace = " \n\r\t";
  size_t start = str.find_first_not_of(whitespace);
  size_t end = str.find_last_not_of(whitespace);
  return (start == std::string::npos) ? "" : str.substr(start, end - start + 1);
}

std::unordered_map<std::string, std::string> Preprocessor::ParseAttributes(const std::string& attrString) {
  std::unordered_map<std::string, std::string> attributes;
  size_t i = 0;
  size_t len = attrString.length();

  while (i < len) {
    while (i < len && std::isspace(attrString[i])) {
      i++;
    }

    if (i >= len) break;

    size_t nameStart = i;
    while (i < len && (std::isalnum(attrString[i]) || attrString[i] == '-' || attrString[i] == '_')) {
      i++;
    }
    size_t nameEnd = i;
    std::string attrName = attrString.substr(nameStart, nameEnd - nameStart);

    while (i < len && std::isspace(attrString[i])) {
      i++;
    }

    if (i < len && attrString[i] == '=') {
      i++;

      while (i < len && std::isspace(attrString[i])) {
        i++;
      }

      if (i >= len) {
        attributes[attrName] = "";
        break;
      }

      std::string attrValue;

      if (attrString[i] == '{') {
        int braceCount = 1;
        i++;
        size_t valueStart = i;
        while (i < len && braceCount > 0) {
          if (attrString[i] == '{')
            braceCount++;
          else if (attrString[i] == '}')
            braceCount--;
          i++;
        }
        size_t valueEnd = (braceCount == 0) ? i - 1 : len;
        attrValue = "{" + attrString.substr(valueStart, valueEnd - valueStart) + "}";
      } else if (attrString[i] == '"' || attrString[i] == '\'') {
        char quoteType = attrString[i];
        i++;
        size_t valueStart = i;
        while (i < len && attrString[i] != quoteType) {
          if (attrString[i] == '\\' && (i + 1) < len && attrString[i + 1] == quoteType) {
            i += 2;
          } else {
            i++;
          }
        }
        size_t valueEnd = (i < len) ? i : len;
        attrValue = attrString.substr(valueStart, valueEnd - valueStart);
        if (i < len && attrString[i] == quoteType) {
          i++;
        }
      } else {
        size_t valueStart = i;
        while (i < len && !std::isspace(attrString[i])) {
          i++;
        }
        size_t valueEnd = i;
        attrValue = attrString.substr(valueStart, valueEnd - valueStart);
      }

      attributes[attrName] = attrValue;
    } else {
      attributes[attrName] = "";
    }
  }

  return attributes;
}

Preprocessor::DOMNode Preprocessor::ParseHTML(const std::string& html, size_t& pos) {
  DOMNode node;
  if (html[pos] == '<') {
    size_t tagStart = pos;
    size_t tagEnd = html.find('>', pos);
    if (tagEnd == std::string::npos) {
      node.type = "text";
      node.textContent = html.substr(pos);
      pos = html.size();
      return node;
    }

    std::string tagContent = html.substr(pos + 1, tagEnd - pos - 1);
    bool isClosing = false;

    if (!tagContent.empty() && tagContent[0] == '/') {
      isClosing = true;
      tagContent = tagContent.substr(1);
    }

    bool isSelfClosing = false;
    if (!tagContent.empty() && tagContent.back() == '/') {
      isSelfClosing = true;
      tagContent.pop_back();
      tagContent = Trim(tagContent);
    }

    size_t spacePos = tagContent.find_first_of(" \t\r\n");
    std::string tagName;
    std::string attrString;

    if (spacePos != std::string::npos) {
      tagName = tagContent.substr(0, spacePos);
      attrString = tagContent.substr(spacePos + 1);
    } else {
      tagName = tagContent;
      attrString = "";
    }

    tagName = Trim(tagName);
    attrString = Trim(attrString);

    std::transform(tagName.begin(), tagName.end(), tagName.begin(), ::tolower);

    if (isClosing) {
      node.type = "closing";
      node.tagName = tagName;
      pos = tagEnd + 1;
      std::cout << "Encountered closing tag: </" << tagName << ">" << std::endl;
      return node;
    } else {
      node.type = "element";
      node.tagName = tagName;
      node.attributes = ParseAttributes(attrString);
      pos = tagEnd + 1;

      if (isSelfClosing) {
        std::cout << "Encountered self-closing tag: <" << tagName << " />" << std::endl;
        return node;
      }

      while (pos < html.size()) {
        if (html[pos] == '<') {
          if (pos + 1 < html.size() && html[pos + 1] == '/') {
            size_t closingTagStart = pos;
            size_t closingTagEnd = html.find('>', pos);
            if (closingTagEnd == std::string::npos) {
              break;
            }
            std::string closingTagContent = html.substr(pos + 2, closingTagEnd - pos - 2);
            closingTagContent = Trim(closingTagContent);
            std::transform(closingTagContent.begin(), closingTagContent.end(), closingTagContent.begin(), ::tolower);
            if (closingTagContent == tagName) {
              pos = closingTagEnd + 1;
              std::cout << "Found matching closing tag: </" << tagName << ">" << std::endl;
              break;
            } else {
              DOMNode textNode;
              size_t textStart = pos;
              pos = closingTagEnd + 1;
              textNode.type = "text";
              textNode.textContent = html.substr(textStart, pos - textStart);
              node.children.push_back(textNode);
              std::cout << "Added text node (mismatched closing tag): \"" << textNode.textContent << "\"" << std::endl;
            }
          } else {
            DOMNode child = ParseHTML(html, pos);
            if (child.type != "closing") {
              node.children.push_back(child);
            }
          }
        } else {
          size_t textStart = pos;
          size_t nextTag = html.find('<', pos);
          if (nextTag == std::string::npos) nextTag = html.size();
          std::string text = html.substr(textStart, nextTag - textStart);
          pos = nextTag;
          text = Trim(text);
          if (!text.empty()) {
            DOMNode textNode;
            textNode.type = "text";
            textNode.textContent = text;
            node.children.push_back(textNode);
            std::cout << "Added text node: \"" << text << "\"" << std::endl;
          }
        }
      }
    }
  }
  return node;
}

std::string Preprocessor::GenerateJSON(const DOMNode& node, int indent) {
  std::ostringstream os;
  const std::string indentation(indent, ' ');
  if (node.type == "text") {
    std::string text = node.textContent;
    std::regex cppEscapeRegex(R"(\{([^}]+)\})");
    std::smatch match;
    size_t lastPos = 0;
    bool first = true;

    while (std::regex_search(text.cbegin() + lastPos, text.cend(), match, cppEscapeRegex)) {
      size_t matchPos = match.position(0);
      std::string before = text.substr(lastPos, matchPos);
      if (!before.empty()) {
        if (!first) os << ", ";
        os << "\"" << Trim(before) << "\"";
        first = false;
      }
      if (!first) os << ", ";
      os << Trim(match[1].str());
      first = false;
      lastPos += matchPos + match.length(0);
    }
    std::string after = text.substr(lastPos);
    if (!after.empty()) {
      if (!first) os << ", ";
      os << "\"" << Trim(after) << "\"";
    }
  } else if (node.type == "element") {
    os << indentation << "JSON {\n";
    os << indentation << "    \"" << node.tagName << "\", {\n";

    bool firstAttr = true;
    for (const auto& attr : node.attributes) {
      if (attr.first == "children") continue;
      if (!firstAttr) {
        os << ",\n";
      }
      firstAttr = false;
      os << indentation << "        \"" << attr.first << "\", ";

      std::string attrVal = attr.second;

      if (attrVal.size() >= 4 && attrVal.substr(0, 2) == "{{" && attrVal.substr(attrVal.size() - 2, 2) == "}}") {
        std::string inner = attrVal.substr(1, attrVal.size() - 2);
        os << inner;
      } else if (attrVal.size() >= 2 && attrVal[0] == '{' && attrVal[attrVal.size() - 1] == '}') {
        std::string inner = attrVal.substr(1, attrVal.size() - 2);
        os << inner;
      } else {
        os << "\"" << Trim(attrVal) << "\"";
      }
    }

    if (!node.attributes.empty()) {
      os << ",\n";
    }

    os << indentation << "        \"children\", JSON::Array {\n";
    for (size_t i = 0; i < node.children.size(); ++i) {
      os << GenerateJSON(node.children[i], indent + 12);
      if (i != node.children.size() - 1) {
        os << ",";
      }
      os << "\n";
    }
    os << indentation << "        }\n";
    os << indentation << "    }\n";
    os << indentation << "}";
  }

  return os.str();
}

std::string Preprocessor::CorrectIndentation(const std::string& code) {
  std::stringstream inputStream(code);
  std::string line, formattedCode;
  int indentLevel = 0;

  while (std::getline(inputStream, line)) {
    size_t firstNonSpace = line.find_first_not_of(" \t");
    if (firstNonSpace != std::string::npos) {
      line = line.substr(firstNonSpace);
    }

    if (line.empty()) continue;

    if (line[0] == '}') {
      indentLevel = std::max(indentLevel - 1, 0);
    }

    formattedCode += std::string(indentLevel * 2, ' ') + line + "\n";

    if (line.back() == '{' && line.find('}') == std::string::npos) {
      indentLevel++;
    }
  }

  return formattedCode;
}

std::vector<std::string> Preprocessor::ExtractValidHtmlBlocks(const std::string& script) {
  std::vector<std::string> validBlocks;
  std::stack<std::string> tagStack;
  std::regex tagRegex(R"(<(/?)(\w+)([^>]*)>)");
  std::smatch match;

  size_t pos = 0;
  while (std::regex_search(script.begin() + pos, script.end(), match, tagRegex)) {
    std::string fullMatch = match[0];
    std::string slash = match[1];
    std::string tagName = match[2];
    std::string attributes = match[3];
    size_t matchPos = match.position(0) + pos;
    pos += match.position(0) + match.length(0);

    std::transform(tagName.begin(), tagName.end(), tagName.begin(), ::tolower);

    if (htmlTags.find(tagName) == htmlTags.end()) {
      continue;
    }

    if (slash.empty()) {
      tagStack.push(tagName);
      size_t blockStart = matchPos;
      std::string closingTag = "</" + tagName + ">";
      size_t closingPos = script.find(closingTag, pos);
      if (closingPos != std::string::npos) {
        size_t blockEnd = closingPos + closingTag.length();
        std::string block = script.substr(blockStart, blockEnd - blockStart);
        validBlocks.push_back(block);
        pos = blockEnd;
        tagStack.pop();
        std::cout << "Extracted block for <" << tagName << ">" << std::endl;
      }
    }
  }

  return validBlocks;
}

std::string Preprocessor::AddHeader(const std::string& script) {
  return header + "\n" + script;
}

std::string Preprocessor::Process(const std::string& input) {
  std::string script = input;

  std::vector<std::string> htmlBlocks = ExtractValidHtmlBlocks(script);

  std::unordered_map<std::string, std::string> replacements;

  for (const auto& html : htmlBlocks) {
    size_t pos = 0;
    DOMNode root = ParseHTML(html, pos);
    std::string jsonResult = GenerateJSON(root, 0);
    jsonResult = CorrectIndentation(jsonResult);
    replacements[html] = "\n" + jsonResult;
  }

  for (const auto& [htmlBlock, jsonString] : replacements) {
    size_t pos = script.find(htmlBlock);
    if (pos != std::string::npos) {
      script.replace(pos, htmlBlock.length(), jsonString);
    }
  }

  std::string finalScript = AddHeader(script);

  return finalScript;
}
