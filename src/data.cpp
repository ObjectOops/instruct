#include "data.hpp"

Instruct::Data::Data(const std::filesystem::path &filePath) : 
    filePath {filePath}, 
    data {YAML::LoadFile(filePath)} 
{
}

void Instruct::Data::saveData() {
    std::ofstream fout {};
    fout.exceptions(std::ofstream::failbit | std::ofstream::badbit);
    fout.open(filePath);
    fout << data;
    fout.close();    
}
