#include <fstream>
#include <vector>

#define LOGURU_WITH_STREAMS 1
#include "loguru.hpp"

#include "constants.hpp"
#include "data.hpp"

namespace instruct {

namespace keys {
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
}

void Data::initEmpty() {
    IData::instructorData = std::make_unique<IData>();
    IData::instructorData->filePath = constants::INSTRUCTOR_CONFIG;
    SData::studentsData = std::make_unique<SData>();
    SData::studentsData->filePath = constants::STUDENTS_CONFIG;
}

void Data::initAll() {
    IData::instructorData = std::make_unique<IData>(constants::INSTRUCTOR_CONFIG);
    SData::studentsData = std::make_unique<SData>(constants::STUDENTS_CONFIG);
    
    LOG_S(1) << "Instructor config data: \n" << IData::instructorData->yaml;
    LOG_S(1) << "Students config data: \n" << SData::studentsData->yaml;
}

void Data::saveAll() {
    IData::instructorData->saveData();
    SData::studentsData->saveData();
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
    for (const std::pair<uuids::uuid, Student> &studentPair : students) {
        studentVec.push_back(studentPair.second);
    }
    yaml[keys::STUDENTS] = studentVec;
    
    Data::saveData();
}

}

namespace YAML {

using instruct::SData;
namespace keys = instruct::keys;

template<>
struct convert<SData::Student> {
    static Node encode(const SData::Student &rhs) {
        Node node;
        node[keys::UUID] = uuids::to_string(rhs.uuid);
        node[keys::DISPLAY_NAME] = rhs.displayName;
        node[keys::PASSWORD_SHA256] = rhs.pswdSHA256;
        node[keys::PASSWORD_SALT] = rhs.pswdSalt;
        node[keys::ELEVATED_PRIVILEGES] = rhs.elevatedPriveleges;
        return node;
    }
    static bool decode(const Node &node, SData::Student &rhs) {
        rhs.uuid = uuids::uuid::from_string(node[keys::UUID].as<std::string>()).value();
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

}
