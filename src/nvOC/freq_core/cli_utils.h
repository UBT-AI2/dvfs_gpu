//
// Created by Alex on 16.06.2018.
//
#pragma once

#include <string>
#include <map>

namespace frequency_scaling {

    std::string cli_get_string(const std::string &user_msg, const std::string &regex_to_match);

    int cli_get_int(const std::string &user_msg);

    double cli_get_float(const std::string &user_msg);

    std::map<std::string, std::string> parse_cmd_options(int argc, char **argv);

}
