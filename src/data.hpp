#ifndef INSTRUCT_DATA_HPP
#define INSTRUCT_DATA_HPP

#include <filesystem>
#include <fstream>
#include <memory>

#include "yaml-cpp/yaml.h"

#include "constants.hpp"

namespace instruct {
    class Data {
        private:
        std::filesystem::path filePath;
        
        public:
        YAML::Node data;

        Data() = delete;
        // Throws `YAML::Exception` on failure.
        Data(const std::filesystem::path &);
        
        // Throws `std::ios_base::failure` on failure.
        void saveData();
        
        inline static std::unique_ptr<Data> instructData;
        
        static void initAll();
    };
}

#endif
