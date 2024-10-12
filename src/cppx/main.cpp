#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "cppx/preprocessor.hpp"

std::string read_file(const std::filesystem::path& file_path) {
  std::ifstream file(file_path, std::ios::in | std::ios::binary);
  if (!file) {
    throw std::runtime_error("Failed to open file: " + file_path.string());
  }
  return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

void write_file(const std::filesystem::path& file_path, const std::string& content) {
  std::ofstream file(file_path, std::ios::out | std::ios::binary);
  if (!file) {
    throw std::runtime_error("Failed to write to file: " + file_path.string());
  }
  file.write(content.c_str(), content.size());
}

void build() {
  const std::filesystem::path original_src_dir = "src";
  const std::filesystem::path original_router_dir = "router";
  const std::filesystem::path cppx_dir = ".cppx";
  const std::filesystem::path cppx_src_dir = cppx_dir / "src";
  const std::filesystem::path cppx_router_dir = cppx_dir / "router";
  const std::filesystem::path cppx_include_dir = cppx_dir / "include";
  const std::filesystem::path build_lib_dir = cppx_dir / "build" / "lib";
  const std::filesystem::path build_bin_dir = cppx_dir / "build" / "bin";
  const std::string lib_name = "libcppx.a";

  std::filesystem::create_directories(cppx_src_dir);
  std::filesystem::create_directories(cppx_router_dir);
  std::filesystem::create_directories(cppx_include_dir);
  std::filesystem::create_directories(build_lib_dir);
  std::filesystem::create_directories(build_bin_dir);

  const std::filesystem::path original_include_dir = "include";
  if (std::filesystem::exists(original_include_dir) && std::filesystem::is_directory(original_include_dir)) {
    if (std::filesystem::exists(cppx_include_dir)) {
      std::filesystem::remove_all(cppx_include_dir);
    }
    std::filesystem::copy(original_include_dir, cppx_include_dir, std::filesystem::copy_options::recursive);
    std::cout << "Copied include directory to " << cppx_include_dir << std::endl;
  } else {
    std::cerr << "Warning: Original include directory " << original_include_dir << " does not exist or is not a directory." << std::endl;
  }

  std::vector<std::filesystem::path> lib_cpp_files;
  std::vector<std::pair<std::filesystem::path, std::filesystem::path>> source_dirs = {{original_src_dir, cppx_src_dir}, {original_router_dir, cppx_router_dir}};

  for (const auto& [original_dir, cppx_output_dir] : source_dirs) {
    if (!std::filesystem::exists(original_dir) || !std::filesystem::is_directory(original_dir)) {
      std::cerr << "Warning: Source directory " << original_dir << " does not exist or is not a directory." << std::endl;
      continue;
    }

    for (const auto& file : std::filesystem::recursive_directory_iterator(original_dir)) {
      if (file.is_regular_file()) {
        auto ext = file.path().extension().string();
        auto relative_path = std::filesystem::relative(file.path(), original_dir);
        if (ext == ".cpp") {
          std::filesystem::path destination_path = cppx_output_dir / relative_path;
          std::filesystem::create_directories(destination_path.parent_path());
          std::filesystem::copy_file(file.path(), destination_path, std::filesystem::copy_options::overwrite_existing);
          std::cout << "Copied .cpp: " << file.path() << " -> " << destination_path << std::endl;
          lib_cpp_files.push_back(destination_path);
        } else if (ext == ".cppx") {
          std::filesystem::path output_cpp_path = cppx_output_dir / relative_path;
          output_cpp_path.replace_extension(".cpp");
          std::filesystem::create_directories(output_cpp_path.parent_path());
          std::string input_script = read_file(file.path());
          std::string transformed_script = Preprocessor::Process(input_script);
          write_file(output_cpp_path, transformed_script);
          std::cout << "Processed .cppx: " << file.path() << " -> " << output_cpp_path << std::endl;
          lib_cpp_files.push_back(output_cpp_path);
        }
      }
    }
  }

  if (lib_cpp_files.empty()) {
    std::cerr << "Error: No source files (.cpp or .cppx) found to compile." << std::endl;
    return;
  }

  std::vector<std::filesystem::path> lib_object_files;

  std::cout << "Building library..." << std::endl;

  for (const auto& cpp_file : lib_cpp_files) {
    std::filesystem::path relative_path = std::filesystem::relative(cpp_file, cppx_dir);
    std::filesystem::path object_path = build_lib_dir / relative_path;
    object_path.replace_extension(".o");

    std::filesystem::create_directories(object_path.parent_path());

    std::string compile_cmd = "g++ -c \"" + cpp_file.string() + "\" -I\".cppx/include\" -std=c++20 -O3 -o \"" + object_path.string() + "\"";
    if (std::system(compile_cmd.c_str()) != 0) {
      std::cerr << "Error: Compilation failed for " << cpp_file << std::endl;
      return;
    }

    lib_object_files.push_back(object_path);
  }

  std::cout << "Archiving library..." << std::endl;

  if (!lib_object_files.empty()) {
    std::filesystem::path archive_path = build_lib_dir / lib_name;
    std::string archive_cmd = "ar rcs \"" + archive_path.string() + "\"";
    for (const auto& obj : lib_object_files) {
      archive_cmd += " \"" + obj.string() + "\"";
    }
    if (std::system(archive_cmd.c_str()) != 0) {
      std::cerr << "Error: Archiving failed for " << lib_name << std::endl;
      return;
    }
    std::cout << "Library archived at " << archive_path << std::endl;
  } else {
    std::cerr << "Error: No object files to archive." << std::endl;
    return;
  }

  std::cout << "Build completed successfully!" << std::endl;
}

void watch() {
  std::unordered_map<std::filesystem::path, std::filesystem::file_time_type> files_last_write_time;
  std::vector<std::filesystem::path> dirs_to_watch = {"src", "router", "include"};

  for (const auto& dir : dirs_to_watch) {
    if (std::filesystem::exists(dir) && std::filesystem::is_directory(dir)) {
      for (const auto& file : std::filesystem::recursive_directory_iterator(dir)) {
        if (file.is_regular_file()) {
          files_last_write_time[file.path()] = std::filesystem::last_write_time(file);
        }
      }
    }
  }

  build();
  std::cout << "Watching for changes..." << std::endl;

  while (true) {
    bool files_changed = false;

    for (const auto& dir : dirs_to_watch) {
      if (!std::filesystem::exists(dir) || !std::filesystem::is_directory(dir)) {
        continue;
      }

      for (const auto& file : std::filesystem::recursive_directory_iterator(dir)) {
        if (file.is_regular_file()) {
          auto current_file_time = std::filesystem::last_write_time(file);
          auto it = files_last_write_time.find(file.path());

          if (it == files_last_write_time.end()) {
            files_last_write_time[file.path()] = current_file_time;
            files_changed = true;
            std::cout << "New file detected: " << file.path() << std::endl;
          } else {
            if (it->second != current_file_time) {
              it->second = current_file_time;
              files_changed = true;
              std::cout << "Modified file detected: " << file.path() << std::endl;
            }
          }
        }
      }
    }

    std::vector<std::filesystem::path> files_to_remove;
    for (const auto& [path, _] : files_last_write_time) {
      if (!std::filesystem::exists(path)) {
        files_to_remove.push_back(path);
        files_changed = true;
        std::cout << "Deleted file detected: " << path << std::endl;
      }
    }
    for (const auto& path : files_to_remove) {
      files_last_write_time.erase(path);
    }

    if (files_changed) {
      std::cout << "Changes detected. Rebuilding..." << std::endl;
      build();
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }
}

int main(int argc, char* argv[]) {
  if (argc > 1 && (std::string(argv[1]) == "-w" || std::string(argv[1]) == "--watch")) {
    watch();
  } else {
    build();
  }
  return 0;
}
