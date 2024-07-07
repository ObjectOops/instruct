#ifndef INSTRUCT_DATA_HPP
#define INSTRUCT_DATA_HPP

#include <filesystem>
#include <fstream>
#include <memory>

#include "yaml-cpp/yaml.h"

class InstructData {
    private:
    std::filesystem::path filePath;
    
    public:
    YAML::Node data;

    InstructData() = delete;
    // Throws `YAML::Exception` on failure.
    InstructData(const std::filesystem::path &);
    
    // Throws `std::ios_base::failure` on failure.
    void saveData();
};

inline std::unique_ptr<InstructData> instructData;

#endif
