#ifndef INSTRUCT_SETUP_HPP
#define INSTRUCT_SETUP_HPP

#include <system_error>
#include <filesystem>
#include <exception>
#include <iostream>

#include "yaml-cpp/yaml.h"

#include "constants.hpp"
#include "terminal.hpp"

inline std::error_code setupError;

bool dataDirExists();

bool createDataDir();

bool populateDataDir();

void deleteDataDir();

#endif
