#ifndef INSTRUCT_DATA_HPP
#define INSTRUCT_DATA_HPP

#include <filesystem>
#include <memory>

#include "yaml-cpp/yaml.h"

namespace instruct {
    class Data {
        protected:
        std::filesystem::path filePath;
        YAML::Node yaml;

        public:

        // Throws `YAML::Exception` on failure.
        Data() = default;
        Data(const std::filesystem::path &);
        
        // Throws `std::ios_base::failure` on failure.
        virtual void saveData();

        static void initEmpty();
        static void initAll();
    };
    class IData : public Data {
        public:
        IData() = default;
        IData(const std::filesystem::path &);
        void saveData() override;
        
        std::string authHost;
        int authPort;
        int ovscsPort;
        std::string pswdSHA256;
        
        inline static std::unique_ptr<IData> data;
    };
}

#endif
