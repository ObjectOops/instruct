#include <fstream>

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
}

void Data::initAll() {
    IData::instructorData = std::make_unique<IData>(constants::INSTRUCTOR_CONFIG);
    
    LOG_S(INFO) << "Instructor config data: \n" << IData::instructorData->yaml;
}

void Data::saveAll() {
    IData::instructorData->saveData();
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

}
