#pragma once

#include <cstddef>
#include "jv_parse_json.h"
#include <string>
#include <map>
#include <vector>

void parse_json_no_array(const char* my_json_string,
                         const std::vector<std::string> &keys,
                         std::map<std::string, std::string> &res);
