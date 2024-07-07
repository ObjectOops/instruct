#include "data.hpp"

instruct::Data::Data(const std::filesystem::path &filePath) : 
    filePath {filePath}, 
    data {YAML::LoadFile(filePath)} 
{
}

void instruct::Data::saveData() {
    std::ofstream fout {};
    fout.exceptions(std::ofstream::failbit | std::ofstream::badbit);
    fout.open(filePath);
    fout << data;
    fout.close();
}

void instruct::Data::initAll() {
    instruct::Data::instructData = 
        std::make_unique<instruct::Data>(instruct::constants::INSTRUCTOR_CONFIG);
}
