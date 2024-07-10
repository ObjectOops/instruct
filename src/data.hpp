#ifndef INSTRUCT_DATA_HPP
#define INSTRUCT_DATA_HPP

#include <filesystem>
#include <memory>
#include <string>

#include "yaml-cpp/yaml.h"

#define DATA_ATTR(TYPE, NAME) \
private: \
TYPE NAME; \
public: \
inline const TYPE &get_##NAME() { \
    return NAME; \
} \
inline void set_##NAME(const TYPE &val) { \
    NAME = val; \
    saveData(); \
}

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
        static void saveAll();
    };
    class IData : public Data {
        public:
        IData() = default;
        IData(const std::filesystem::path &);
        void saveData() override;
        
        DATA_ATTR(std::string, authHost)
        DATA_ATTR(int, authPort)
        DATA_ATTR(int, codePort)
        DATA_ATTR(std::string, pswdSHA256)
        DATA_ATTR(std::string, pswdSalt)
        DATA_ATTR(bool, firstTime)
        
        inline static std::unique_ptr<IData> instructorData;
    };
}

#undef DATA_ATTR

#endif
