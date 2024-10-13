#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "cppx/preprocessor.hpp"

std::string readFile(const std::filesystem::path& filePath) {
  std::ifstream inputFile(filePath);
  if (!inputFile) {
    std::cerr << "Error: Cannot open file " << filePath << std::endl;
    return "";
  }
  return std::string((std::istreambuf_iterator<char>(inputFile)), std::istreambuf_iterator<char>());
}

void writeFile(const std::filesystem::path& filePath, const std::string& content) {
  std::ofstream outputFile(filePath);
  if (!outputFile) {
    std::cerr << "Error: Cannot write to file " << filePath << std::endl;
    return;
  }
  outputFile << content;
}

void copyAndProcessFiles(const std::filesystem::path& sourceDir, const std::filesystem::path& destinationDir) {
  if (!std::filesystem::exists(sourceDir)) {
    return;
  }

  for (const auto& entry : std::filesystem::recursive_directory_iterator(sourceDir)) {
    if (entry.is_regular_file()) {
      auto relativePath = std::filesystem::relative(entry.path(), sourceDir);
      auto destinationPath = destinationDir / relativePath;

      if (entry.path().extension() == ".cppx") {
        destinationPath.replace_extension(".cpp");
      }

      try {
        std::filesystem::create_directories(destinationPath.parent_path());
      } catch (const std::filesystem::filesystem_error& error) {
        std::cerr << "Error: Failed to create directories for " << destinationPath << ": " << error.what() << std::endl;
        continue;
      }

      std::string fileContent = readFile(entry.path());

      std::string warning = "// Warning: This is a generated file. Do not modify directly.\n";

      if (entry.path().extension() == ".cppx") {
        std::string processedContent = Preprocessor::Process(fileContent);
        fileContent = warning + processedContent;
      } else {
        fileContent = warning + fileContent;
      }

      if (destinationDir.string().find(".cppx/router") != std::string::npos) {
        std::regex functionPattern(R"(\bPage\s+(\w+)\s*\()");
        std::smatch match;

        if (std::regex_search(fileContent, match, functionPattern) && match.size() > 1) {
          std::string pageName = match[1].str();
          fileContent = std::regex_replace(fileContent, functionPattern, "Page " + pageName + "(");
          std::string externDeclaration = "extern \"C\" PageFunction getPageFunction() { return &" + pageName + "; }\n";
          fileContent += "\n" + externDeclaration;
        }
      }

      writeFile(destinationPath, fileContent);
    }
  }
}

void build() {
  const std::string projectAlias = "example";
  const std::filesystem::path includeSourceDir = "include/" + projectAlias;
  const std::filesystem::path routerSourceDir = "router/" + projectAlias;
  const std::filesystem::path srcSourceDir = "src/" + projectAlias;

  const std::filesystem::path includeDestinationDir = ".cppx/include/" + projectAlias;
  const std::filesystem::path routerDestinationDir = ".cppx/router/" + projectAlias;
  const std::filesystem::path srcDestinationDir = ".cppx/src/" + projectAlias;

  copyAndProcessFiles(includeSourceDir, includeDestinationDir);
  copyAndProcessFiles(routerSourceDir, routerDestinationDir);
  copyAndProcessFiles(srcSourceDir, srcDestinationDir);

  const std::filesystem::path mainCppPath = ".cppx/src/main.cpp";
  std::filesystem::create_directories(mainCppPath.parent_path());

  std::ostringstream mainContentStream;
  mainContentStream << "// Warning: This is a generated file. Do not modify directly.\n"
                    << "#include <iostream>\n"
                    << "#include <dlfcn.h>\n"
                    << "#include <string>\n"
                    << "#include \"cppx/json.hpp\"\n"
                    << "#include \"cppx/page.hpp\"\n"
                    << "\n"
                    << "int main() {\n"
                    << "    std::string input;\n"
                    << "    std::cout << \"> \";\n"
                    << "    std::cin >> input;\n"
                    << "\n"
                    << "    std::string sharedObjectPath = \".cppx/build/" << projectAlias << "/bin/router/" << projectAlias << "/\" + input + \"/page.so\";\n"
                    << "    void* handle = dlopen(sharedObjectPath.c_str(), RTLD_NOW);\n"
                    << "    if (!handle) {\n"
                    << "        std::cerr << \"404 Page Not Found\" << std::endl;\n"
                    << "        return 1;\n"
                    << "    }\n"
                    << "\n"
                    << "    typedef PageFunction (*GetPageFunction)();\n"
                    << "    GetPageFunction getPageFunction = (GetPageFunction)dlsym(handle, \"getPageFunction\");\n"
                    << "    if (!getPageFunction) {\n"
                    << "        std::cerr << \"404 Page Not Found\" << std::endl;\n"
                    << "        dlclose(handle);\n"
                    << "        return 1;\n"
                    << "    }\n"
                    << "\n"
                    << "    PageFunction pageFunction = getPageFunction();\n"
                    << "    {\n"
                    << "        JSON page = pageFunction();\n"
                    << "        std::cout << page << std::endl;\n"
                    << "    }\n"
                    << "\n"
                    << "    dlclose(handle);\n"
                    << "    return 0;\n"
                    << "}\n";

  std::string mainContent = mainContentStream.str();
  writeFile(mainCppPath, mainContent);

  const std::filesystem::path buildBaseDir = ".cppx/build/" + projectAlias;
  const std::filesystem::path buildLibDir = buildBaseDir / "lib";
  const std::filesystem::path buildBinDir = buildBaseDir / "bin";

  std::filesystem::create_directories(buildLibDir);
  std::filesystem::create_directories(buildBinDir);

  std::vector<std::filesystem::path> librarySources;

  for (const auto& entry : std::filesystem::recursive_directory_iterator(".cppx/src")) {
    if (entry.is_regular_file()) {
      if (entry.path().extension() == ".cpp" || entry.path().extension() == ".c") {
        if (entry.path().filename() != "main.cpp") {
          librarySources.push_back(entry.path());
        }
      }
    }
  }

  std::vector<std::filesystem::path> libraryObjectFiles;
  bool hasLibrary = !librarySources.empty();

  if (hasLibrary) {
    for (const auto& sourceFile : librarySources) {
      auto relativePath = std::filesystem::relative(sourceFile, ".cppx");
      auto objectPath = buildLibDir / relativePath;
      objectPath.replace_extension(".o");

      std::filesystem::create_directories(objectPath.parent_path());

      std::string compileCommand = "g++ -c \"" + sourceFile.string() + "\" -I\".cppx/include\" -I\"include\" -std=c++20 -O3 -fPIC -o \"" + objectPath.string() + "\"";
      if (std::system(compileCommand.c_str()) != 0) {
        std::cerr << "Error: Compilation failed for " << sourceFile << std::endl;
        return;
      }

      libraryObjectFiles.push_back(objectPath);
    }

    std::filesystem::path archivePath = buildLibDir / ("lib" + projectAlias + ".a");
    std::string archiveCommand = "ar rcs \"" + archivePath.string() + "\"";
    for (const auto& objectFile : libraryObjectFiles) {
      archiveCommand += " \"" + objectFile.string() + "\"";
    }

    if (std::system(archiveCommand.c_str()) != 0) {
      std::cerr << "Error: Archiving failed for " << archivePath << std::endl;
      return;
    }
  }

  auto mainObjectPath = buildBinDir / "main.o";
  std::string mainCompileCommand = "g++ -c \"" + mainCppPath.string() + "\" -I\".cppx/include\" -I\"include\" -std=c++20 -O3 -o \"" + mainObjectPath.string() + "\"";
  if (std::system(mainCompileCommand.c_str()) != 0) {
    std::cerr << "Error: Compilation failed for main.cpp" << std::endl;
    return;
  }

  auto mainExecutablePath = buildBinDir / "main";
  std::string mainLinkCommand = "g++ \"" + mainObjectPath.string() + "\"";

  if (hasLibrary) {
    mainLinkCommand += " -L\"" + buildLibDir.string() + "\" -l" + projectAlias;
  }

  mainLinkCommand += " -L\"build/cppx/lib\" -lcppx -ldl -std=c++20 -O3 -o \"" + mainExecutablePath.string() + "\"";

  if (std::system(mainLinkCommand.c_str()) != 0) {
    std::cerr << "Error: Linking failed for main executable" << std::endl;
    return;
  }

  for (const auto& entry : std::filesystem::recursive_directory_iterator(".cppx/router")) {
    if (entry.is_regular_file()) {
      if (entry.path().extension() == ".cpp") {
        auto relativePath = std::filesystem::relative(entry.path(), ".cppx");
        auto sharedObjectDir = buildBinDir / relativePath.parent_path();
        auto sharedObjectPath = sharedObjectDir / "page.so";

        std::filesystem::create_directories(sharedObjectDir);

        std::string buildCommand = "g++ -fPIC -shared \"" + entry.path().string() + "\" -I\".cppx/include\" -I\"include\" -std=c++20 -O3";

        if (hasLibrary) {
          buildCommand += " -L\"" + buildLibDir.string() + "\" -l" + projectAlias;
        }

        buildCommand += " -L\"build/cppx/lib\" -lcppx -o \"" + sharedObjectPath.string() + "\"";

        if (std::system(buildCommand.c_str()) != 0) {
          std::cerr << "Error: Building bundle failed for " << entry.path() << std::endl;
          return;
        }
      }
    }
  }

  std::cout << "Build completed successfully!" << std::endl;
}

int main(int argc, char* argv[]) {
  build();
  return 0;
}
