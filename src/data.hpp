#ifndef INSTRUCT_DATA_HPP
#define INSTRUCT_DATA_HPP

#include <unordered_map>
#include <unordered_set>
#include <filesystem>
#include <memory>
#include <string>
#include <set>

#include "yaml-cpp/yaml.h"
#include "uuid.h"

#define DATA_ATTR_BASE(QUALIFIERS, TYPE, NAME) \
private: \
TYPE NAME; \
public: \
inline QUALIFIERS TYPE &get_##NAME() { \
    return NAME; \
} \
inline void set_##NAME(const TYPE &val) { \
    NAME = val; \
    saveData(); \
}

#define SINGLE(...) __VA_ARGS__

#define DATA_ATTR(TYPE, NAME) \
DATA_ATTR_BASE(const, SINGLE(TYPE), NAME)

#define DATA_ATTR_REF_MUTABLE(TYPE, NAME) \
DATA_ATTR_BASE(, SINGLE(TYPE), NAME)

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

    class SData : public Data {
        public:
        SData() = default;
        SData(const std::filesystem::path &);
        void saveData() override;
        
        DATA_ATTR(std::string, authHost)
        DATA_ATTR(int, authPort)
        DATA_ATTR(std::set<int>, codePorts)
        DATA_ATTR(SINGLE(std::pair<int, int>), codePortRange)
        DATA_ATTR(bool, useRandomPorts)
        
        struct Student {
            uuids::uuid uuid;
            std::string displayName;
            std::string pswdSHA256;
            std::string pswdSalt;
            bool elevatedPriveleges;
        };
        DATA_ATTR(SINGLE(std::unordered_map<uuids::uuid, Student>), students)
        
        inline static std::unique_ptr<SData> studentsData;
        
        static bool importStudentsList(
            const std::filesystem::path &, 
            const std::string &, 
            const std::string &, 
            const std::string &, 
            bool
        );
    };

    class TData : public Data {
        public:
        TData() = default;
        TData(const std::filesystem::path &);
        void saveData() override;
        
        DATA_ATTR_REF_MUTABLE(std::unordered_set<uuids::uuid>, selectedTestUUIDs)
        struct TestCase {
            uuids::uuid uuid;
            std::string displayName;
            std::string instructorRunCmd;
            std::string studentRunCmd;
            double secondsAllotted;
        };
        DATA_ATTR(SINGLE(std::unordered_map<uuids::uuid, TestCase>), tests)
        
        inline static std::unique_ptr<TData> testsData;
    };
    
    class UData : public Data {
        public:
        UData() = default;
        UData(const std::filesystem::path &);
        void saveData() override;
        
        DATA_ATTR(bool, alwaysShowStudentUUIDs)
        DATA_ATTR(bool, alwaysShowTestUUIDs)
        DATA_ATTR_REF_MUTABLE(int, studentPaneWidth)
        
        inline static std::unique_ptr<UData> uiData;
    };
}

#undef DATA_ATTR_BASE
#undef DATA_ATTR
#undef DATA_ATTR_REF_MUTABLE
#undef SINGLE

#endif
