#include <fstream>

#define LOGURU_WITH_STREAMS 1
#include "loguru.hpp"

#include "constants.hpp"
#include "data.hpp"

using namespace instruct;

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
    IData::data = std::make_unique<IData>();
    IData::data->filePath = constants::INSTRUCTOR_CONFIG;
}

void Data::initAll() {
    IData::data = std::make_unique<IData>(constants::INSTRUCTOR_CONFIG);
    
    LOG_S(INFO) << "Instructor config data: \n" << IData::data->yaml;
}

IData::IData(const std::filesystem::path &filePath) : Data {filePath} {
    authHost = yaml["auth_host"].as<std::string>();
    authPort = yaml["auth_port"].as<int>();
    ovscsPort = yaml["openvscode_server_port"].as<int>();
    pswdSHA256 = yaml["password_sha256"].as<std::string>();
}

void IData::saveData() {
    yaml["auth_host"] = authHost;
    yaml["auth_port"] = authPort;
    yaml["openvscode_server_port"] = ovscsPort;
    yaml["password_sha256"] = pswdSHA256;
    
    Data::saveData();
}
