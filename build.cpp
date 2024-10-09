#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

int main() {
  const std::string include_dir = "include";
  const std::string src_dir = "src";
  const std::string build_lib_dir = "lib";
  const std::string build_bin_dir = "bin";
  const std::string lib_name = "libcppx.a";

  if (std::system(("mkdir -p " + build_lib_dir).c_str()) != 0) {
    std::cerr << "Error: Failed to create library build directory" << std::endl;
    return 1;
  }

  if (std::system(("mkdir -p " + build_bin_dir).c_str()) != 0) {
    std::cerr << "Error: Failed to create binary build directory" << std::endl;
    return 1;
  }

  std::vector<std::filesystem::path> lib_cpp_files = {
    "src/cppx/json.cpp",
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

    if (std::system(("mkdir -p " + object_path.parent_path().string()).c_str()) != 0) {
      std::cerr << "Error: Failed to create directory for " << object_path << std::endl;
      return 1;
    }

    if (std::system(("g++ -c \"" + cpp_file.string() + "\" -I\"" + include_dir + "\" -std=c++20 -O3 -o \"" + object_path.string() + "\"").c_str()) != 0) {
      std::cerr << "Error: Compilation failed for " << cpp_file << std::endl;
      return 1;
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
      return 1;
    }
  } else {
    std::cerr << "Error: No library source files found to archive." << std::endl;
    return 1;
  }

  std::cout << "Building executables..." << std::endl;

  for (const auto &exe_src : exe_sources) {
    std::filesystem::path relative_path = std::filesystem::relative(exe_src, src_dir);
    std::filesystem::path exe_obj_path = build_bin_dir / relative_path;
    exe_obj_path.replace_extension(".o");

    if (std::system(("mkdir -p " + exe_obj_path.parent_path().string()).c_str()) != 0) {
      std::cerr << "Error: Failed to create directory for " << exe_obj_path << std::endl;
      return 1;
    }

    if (std::system(("g++ -c \"" + exe_src.string() + "\" -I\"" + include_dir + "\" -std=c++20 -O3 -o \"" + exe_obj_path.string() + "\"").c_str()) != 0) {
      std::cerr << "Error: Compilation failed for executable source " << exe_src << std::endl;
      return 1;
    }

    std::filesystem::path exe_output_path = exe_obj_path;
    exe_output_path.replace_extension("");

    if (std::system(("g++ \"" + exe_obj_path.string() + "\" -L\"" + build_lib_dir + "\" -lcppx -std=c++20 -O3 -o \"" + exe_output_path.string() + "\"").c_str()) != 0) {
      std::cerr << "Error: Linking failed for executable " << exe_output_path << std::endl;
      return 1;
    }
  }

  std::cout << "Build completed successfully!" << std::endl;
  return 0;
}
