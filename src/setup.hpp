#ifndef INSTRUCT_SETUP_HPP
#define INSTRUCT_SETUP_HPP

#include <filesystem>
#include <string>

inline std::error_code setupError;

bool dataDirExists(const std::string &dataDir) {
    bool pathExists {std::filesystem::exists(dataDir)};
    bool isDir {std::filesystem::is_directory(dataDir)};
    return pathExists && isDir;
}

bool createDataDir(const std::string &dataDir) {
    return std::filesystem::create_directory(dataDir, setupError);
}

#endif
