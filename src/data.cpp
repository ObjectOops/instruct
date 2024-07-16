#include <stdexcept>
#include <fstream>
#include <vector>

#define LOGURU_WITH_STREAMS 1
#include "loguru.hpp"

#include "constants.hpp"
#include "data.hpp"

namespace instruct {

namespace keys {
    // Instructor and student keys.
    static const std::string AUTH_HOST {"auth_host"};
    static const std::string AUTH_PORT {"auth_port"};
    static const std::string CODE_PORT {"code_port"};
    static const std::string PASSWORD_SHA256 {"password_sha256"};
    static const std::string PASSWORD_SALT {"password_salt"};
    static const std::string FIRST_TIME {"first_time"};
    static const std::string CODE_PORTS {"code_ports"};
    static const std::string CODE_PORT_RANGE {"code_port_range"};
    static const std::string USE_RANDOM_PORTS {"use_random_ports"};
    static const std::string UUID {"uuid"};
    static const std::string DISPLAY_NAME {"display_name"};
    static const std::string ELEVATED_PRIVILEGES {"elevated_privileges"};
    static const std::string STUDENTS {"students"};
    
    // Test keys.
    static const std::string SELECTED_TESTS {"selected_test_uuids"};
    static const std::string TESTS {"tests"};
    static const std::string I_RUN_CMD {"instructor_run_command"};
    static const std::string S_RUN_CMD {"student_run_command"};
    
    // UI keys.
    static const std::string ALWAYS_SHOW_STUDENT_UUIDS {"always_show_student_uuids"};
    static const std::string ALWAYS_SHOW_TEST_UUIDS {"always_show_test_uuids"};
    static const std::string STUDENT_PANE_WIDTH {"student_pane_width"};
}

Data::Data(const std::filesystem::path &filePath) : 
    filePath {filePath}, 
    yaml {YAML::LoadFile(filePath)} 
{
}

void Data::saveData() {
    std::ofstream fout {};
    fout.exceptions(std::ofstream::failbit | std::ofstream::badbit);
    fout.open(filePath);
    fout << yaml;
    fout.close();
    
    DLOG_F(1, "Saved data operation.");
}

void Data::initEmpty() {
    IData::instructorData = std::make_unique<IData>();
    IData::instructorData->filePath = constants::INSTRUCTOR_CONFIG;

    SData::studentsData = std::make_unique<SData>();
    SData::studentsData->filePath = constants::STUDENTS_CONFIG;

    TData::testsData = std::make_unique<TData>();
    TData::testsData->filePath = constants::TESTS_CONFIG;

    UData::uiData = std::make_unique<UData>();
    UData::uiData->filePath = constants::UI_CONFIG;    
}

void Data::initAll() {
    IData::instructorData = std::make_unique<IData>(constants::INSTRUCTOR_CONFIG);
    LOG_S(1) << "Instructor config data: \n" << IData::instructorData->yaml;

    SData::studentsData = std::make_unique<SData>(constants::STUDENTS_CONFIG);
    LOG_S(1) << "Students config data: \n" << SData::studentsData->yaml;

    TData::testsData = std::make_unique<TData>(constants::TESTS_CONFIG);
    LOG_S(1) << "Tests config data: \n" << TData::testsData->yaml;
    
    UData::uiData = std::make_unique<UData>(constants::UI_CONFIG);
    LOG_S(1) << "UI config data: \n" << UData::uiData->yaml;
}

void Data::saveAll() {
    IData::instructorData->saveData();
    SData::studentsData->saveData();
    TData::testsData->saveData();
    UData::uiData->saveData();
}

IData::IData(const std::filesystem::path &filePath) : Data {filePath} {
    authHost = yaml[keys::AUTH_HOST].as<std::string>();
    authPort = yaml[keys::AUTH_PORT].as<int>();
    codePort = yaml[keys::CODE_PORT].as<int>();
    pswdSHA256 = yaml[keys::PASSWORD_SHA256].as<std::string>();
    pswdSalt = yaml[keys::PASSWORD_SALT].as<std::string>();
    firstTime = yaml[keys::FIRST_TIME].as<bool>();
}

void IData::saveData() {
    yaml[keys::AUTH_HOST] = authHost;
    yaml[keys::AUTH_PORT] = authPort;
    yaml[keys::CODE_PORT] = codePort;
    yaml[keys::PASSWORD_SHA256] = pswdSHA256;
    yaml[keys::PASSWORD_SALT] = pswdSalt;
    yaml[keys::FIRST_TIME] = firstTime;
    
    Data::saveData();
}

SData::SData(const std::filesystem::path &filePath) : Data {filePath} {
    authHost = yaml[keys::AUTH_HOST].as<std::string>();
    authPort = yaml[keys::AUTH_PORT].as<int>();
    codePorts = yaml[keys::CODE_PORTS].as<std::set<int>>();
    codePortRange = yaml[keys::CODE_PORT_RANGE].as<std::pair<int, int>>();
    useRandomPorts = yaml[keys::USE_RANDOM_PORTS].as<bool>();
    
    std::vector<Student> studentVec {yaml[keys::STUDENTS].as<std::vector<Student>>()};
    students.reserve(studentVec.size());
    for (Student &student : studentVec) {
        if (students.count(student.uuid) > 0) {
            throw std::runtime_error {"Duplicate student UUID."};
        }
        students.emplace(student.uuid, student);
    }
}

void SData::saveData() {
    yaml[keys::AUTH_HOST] = authHost;
    yaml[keys::AUTH_PORT] = authPort;
    yaml[keys::CODE_PORTS] = codePorts;
    yaml[keys::CODE_PORT_RANGE] = codePortRange;
    yaml[keys::USE_RANDOM_PORTS] = useRandomPorts;
    
    std::vector<Student> studentVec {};
    studentVec.reserve(students.size());
    for (auto [uuid, student] : students) {
        studentVec.push_back(student);
    }
    yaml[keys::STUDENTS] = studentVec;
    
    Data::saveData();
}

TData::TData(const std::filesystem::path &filePath) : Data {filePath} {
    selectedTestUUIDs = yaml[keys::SELECTED_TESTS].as<std::unordered_set<uuids::uuid>>();
    
    std::vector<TestCase> testVec {yaml[keys::TESTS].as<std::vector<TestCase>>()};
    tests.reserve(testVec.size());
    for (TestCase &test : testVec) {
        if (tests.count(test.uuid) > 0) {
            throw std::runtime_error {"Duplicate test UUID."};
        }
        tests.emplace(test.uuid, test);
    }
}

void TData::saveData() {
    yaml[keys::SELECTED_TESTS] = selectedTestUUIDs;

    std::vector<TestCase> testVec {};
    testVec.reserve(tests.size());
    for (auto [uuid, test] : tests) {
        testVec.push_back(test);
    }
    yaml[keys::TESTS] = testVec;
    
    Data::saveData();
}

UData::UData(const std::filesystem::path &filePath) : Data {filePath} {
    alwaysShowStudentUUIDs = yaml[keys::ALWAYS_SHOW_STUDENT_UUIDS].as<bool>();
    alwaysShowTestUUIDs = yaml[keys::ALWAYS_SHOW_TEST_UUIDS].as<bool>();
    studentPaneWidth = yaml[keys::STUDENT_PANE_WIDTH].as<int>();
}

void UData::saveData() {
    yaml[keys::ALWAYS_SHOW_STUDENT_UUIDS] = alwaysShowStudentUUIDs;
    yaml[keys::ALWAYS_SHOW_TEST_UUIDS] = alwaysShowTestUUIDs;
    yaml[keys::STUDENT_PANE_WIDTH] = studentPaneWidth;
    
    Data::saveData();
}

}

namespace YAML {

using instruct::SData;
using instruct::TData;
namespace keys = instruct::keys;

template<>
struct convert<uuids::uuid> {
    static Node encode(const uuids::uuid &rhs) {
        return Node {uuids::to_string(rhs)};
    }
    static bool decode(const Node &node, uuids::uuid &rhs) {
        rhs = uuids::uuid::from_string(node.as<std::string>()).value();

        return true;
    }
};

template<>
struct convert<SData::Student> {
    static Node encode(const SData::Student &rhs) {
        Node node;
        node[keys::UUID] = rhs.uuid;
        node[keys::DISPLAY_NAME] = rhs.displayName;
        node[keys::PASSWORD_SHA256] = rhs.pswdSHA256;
        node[keys::PASSWORD_SALT] = rhs.pswdSalt;
        node[keys::ELEVATED_PRIVILEGES] = rhs.elevatedPriveleges;
        return node;
    }
    static bool decode(const Node &node, SData::Student &rhs) {
        rhs.uuid = node[keys::UUID].as<uuids::uuid>();
        rhs.displayName = node[keys::DISPLAY_NAME].as<std::string>();
        rhs.pswdSHA256 = node[keys::PASSWORD_SHA256].as<std::string>();
        rhs.pswdSalt = node[keys::PASSWORD_SALT].as<std::string>();
        rhs.elevatedPriveleges = node[keys::ELEVATED_PRIVILEGES].as<bool>();

        return true;
    }
};

template<>
struct convert<std::set<int>> {
    static Node encode(const std::set<int> &rhs) {
        return Node {std::vector<int> {rhs.begin(), rhs.end()}};
    }
    static bool decode(const Node &node, std::set<int> &rhs) {
        std::vector<int> vec {node.as<std::vector<int>>()};
        rhs.insert(vec.begin(), vec.end());

        return true;
    }
};

template<>
struct convert<TData::TestCase> {
    static Node encode(const TData::TestCase &rhs) {
        Node node;
        node[keys::UUID] = rhs.uuid;
        node[keys::DISPLAY_NAME] = rhs.displayName;
        node[keys::I_RUN_CMD] = rhs.instructorRunCmd;
        node[keys::S_RUN_CMD] = rhs.studentRunCmd;
        return node;
    }
    static bool decode(const Node &node, TData::TestCase &rhs) {
        rhs.uuid = node[keys::UUID].as<uuids::uuid>();
        rhs.displayName = node[keys::DISPLAY_NAME].as<std::string>();
        rhs.instructorRunCmd = node[keys::I_RUN_CMD].as<std::string>();
        rhs.studentRunCmd = node[keys::S_RUN_CMD].as<std::string>();

        return true;
    }
};

template<>
struct convert<std::unordered_set<uuids::uuid>> {
    static Node encode(const std::unordered_set<uuids::uuid> &rhs) {
        return Node {std::vector<uuids::uuid> {rhs.begin(), rhs.end()}};
    }
    static bool decode(const Node &node, std::unordered_set<uuids::uuid> &rhs) {
        std::vector<uuids::uuid> vec {node.as<std::vector<uuids::uuid>>()};
        rhs.insert(vec.begin(), vec.end());

        return true;
    }
};

}
