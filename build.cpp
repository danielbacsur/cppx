#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

void build() {
  const std::string include_dir = "include";
  const std::string src_dir = "src";
  const std::string build_lib_dir = "lib";
  const std::string build_bin_dir = "bin";
  const std::string lib_name = "libcppx.a";

  try {
    std::filesystem::create_directories(build_lib_dir);
    std::filesystem::create_directories(build_bin_dir);
  } catch (const std::filesystem::filesystem_error &e) {
    std::cerr << "Error: Failed to create build directories: " << e.what() << std::endl;
    return;
  }

  std::vector<std::filesystem::path> lib_cpp_files = {
    "src/cppx/json.cpp",
    "src/cppx/preprocessor.cpp"
  };

  std::vector<std::filesystem::path> exe_sources = {
    "src/cppx/main.cpp"
  };

  std::vector<std::filesystem::path> lib_object_files;

  std::cout << "Building library..." << std::endl;

  for (const auto &cpp_file : lib_cpp_files) {
    std::filesystem::path relative_path = std::filesystem::relative(cpp_file, src_dir);
    std::filesystem::path object_path = build_lib_dir / relative_path;
    object_path.replace_extension(".o");

    try {
      std::filesystem::create_directories(object_path.parent_path());
    } catch (const std::filesystem::filesystem_error &e) {
      std::cerr << "Error: Failed to create directory for " << object_path << ": " << e.what() << std::endl;
      return;
    }

    if (std::system(("g++ -c \"" + cpp_file.string() + "\" -I\"" + include_dir + "\" -std=c++20 -O3 -o \"" + object_path.string() + "\"").c_str()) != 0) {
      std::cerr << "Error: Compilation failed for " << cpp_file << std::endl;
      return;
    }

    lib_object_files.push_back(object_path);
  }

  std::cout << "Archiving library..." << std::endl;

  if (!lib_object_files.empty()) {
    std::string archive_cmd = "ar rcs \"" + build_lib_dir + "/" + lib_name + "\"";

    for (const auto &obj : lib_object_files) {
      archive_cmd += " \"" + obj.string() + "\"";
    }

    if (std::system(archive_cmd.c_str()) != 0) {
      std::cerr << "Error: Archiving failed for " << lib_name << std::endl;
      return;
    }
  } else {
    std::cerr << "Error: No library source files found to archive." << std::endl;
    return;
  }

  std::cout << "Building executables..." << std::endl;

  for (const auto &exe_src : exe_sources) {
    std::filesystem::path relative_path = std::filesystem::relative(exe_src, src_dir);
    std::filesystem::path exe_obj_path = build_bin_dir / relative_path;
    exe_obj_path.replace_extension(".o");

    if (std::system(("mkdir -p " + exe_obj_path.parent_path().string()).c_str()) != 0) {
      std::cerr << "Error: Failed to create directory for " << exe_obj_path << std::endl;
      return;
    }

    if (std::system(("g++ -c \"" + exe_src.string() + "\" -I\"" + include_dir + "\" -std=c++20 -O3 -o \"" + exe_obj_path.string() + "\"").c_str()) != 0) {
      std::cerr << "Error: Compilation failed for executable source " << exe_src << std::endl;
      return;
    }

    std::filesystem::path exe_output_path = exe_obj_path;
    exe_output_path.replace_extension("");

    if (std::system(("g++ \"" + exe_obj_path.string() + "\" -L\"" + build_lib_dir + "\" -lcppx -std=c++20 -O3 -o \"" + exe_output_path.string() + "\"").c_str()) != 0) {
      std::cerr << "Error: Linking failed for executable " << exe_output_path << std::endl;
      return;
    }
  }

  std::cout << "Build completed successfully!" << std::endl;
}

void watch() {
  std::unordered_map<std::filesystem::path, std::filesystem::file_time_type> files_last_write_time;

  std::vector<std::string> dirs_to_watch = {
    "include",
    "src"
  };

  for (const auto &dir : dirs_to_watch) {
    for (const auto &file : std::filesystem::recursive_directory_iterator(dir)) {
      if (file.is_regular_file()) {
        files_last_write_time[file.path()] = std::filesystem::last_write_time(file);
      }
    }
  }

  build();

  std::cout << "Watching for changes..." << std::endl;

  while (true) {
    bool files_changed = false;

    for (const auto &dir : dirs_to_watch) {
      for (const auto &file : std::filesystem::recursive_directory_iterator(dir)) {
        if (file.is_regular_file()) {
          auto current_file_time = std::filesystem::last_write_time(file);
          if (files_last_write_time[file.path()] != current_file_time) {
            files_last_write_time[file.path()] = current_file_time;
            files_changed = true;
          }
        }
      }
    }

    if (files_changed) build();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
}

int main(int argc, char *argv[]) {
  if (argc > 1 && (std::string(argv[1]) == "-w" || std::string(argv[1]) == "--watch")) {
    watch();
  } else {
    build();
  }

  return 0;
}
