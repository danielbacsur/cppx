#include <fstream>
#include <iostream>
#include <sstream>

#include "cppx/preprocessor.hpp"

std::string ReadFile(const std::string& filePath) {
  std::ifstream inFile(filePath);
  if (!inFile) {
    std::cerr << "Error: Unable to open input file: " << filePath << std::endl;
    return "";
  }
  std::stringstream buffer;
  buffer << inFile.rdbuf();
  return buffer.str();
}

void WriteFile(const std::string& filePath, const std::string& content) {
  std::ofstream outFile(filePath);
  if (!outFile) {
    std::cerr << "Error: Unable to open output file: " << filePath << std::endl;
    return;
  }
  outFile << content;
}

int main(int argc, char* argv[]) {
  if (argc != 3) {
    std::cerr << "Usage: cppx <input_file> <output_file>" << std::endl;
    return 1;
  }

  std::string inputFilePath = argv[1];
  std::string outputFilePath = argv[2];

  std::string inputScript = ReadFile(inputFilePath);
  std::string transformedScript = Preprocessor::Process(inputScript);

  WriteFile(outputFilePath, transformedScript);

  return 0;
}
