#include "data.hpp"

InstructData::InstructData(const std::filesystem::path &filePath) : 
    filePath {filePath}, 
    data {YAML::LoadFile(filePath)} 
{
}

void InstructData::saveData() {
    std::ofstream fout {};
    fout.exceptions(std::ofstream::failbit | std::ofstream::badbit);
    fout.open(filePath);
    fout << data;
    fout.close();    
}
