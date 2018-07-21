//
// Created by Alex on 16.06.2018.
//
#include "cli_utils.h"

#include <limits>
#include <regex>
#include "../common_header/fullexpr_accum.h"


namespace frequency_scaling {

    std::string cli_get_string(const std::string &user_msg,
                               const std::string &regex_to_match) {
        full_expression_accumulator<>(std::cout) << user_msg << std::endl;
        while (true) {
            std::string str;
            std::cin >> str;
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            if (std::regex_match(str, std::regex(regex_to_match)))
                return str;
            full_expression_accumulator<>(std::cout) << "Invalid input. Try again." << std::endl;
            full_expression_accumulator<>(std::cout) << user_msg << std::endl;
        }
    }


    int cli_get_int(const std::string &user_msg) {
        return std::stoi(cli_get_string(user_msg, "[+-]?[[:digit:]]+"));
    }

    double cli_get_float(const std::string &user_msg) {
        return std::stod(cli_get_string(user_msg, "[-+]?[0-9]*\\.?[0-9]+([eE][-+]?[0-9]+)?"));
    }

    std::map<std::string, std::string> parse_cmd_options(int argc, char **argv) {
        std::map<std::string, std::string> res;
        for (int i = 1; i < argc; i++) {
            std::string arg(argv[i]);
            if (std::regex_match(arg, std::regex("--[\\w-]+(=[\\w-./]+)?"))) {
                std::string::size_type pos = arg.find('=');
                if (pos != std::string::npos) {
                    res.emplace(arg.substr(0, pos), arg.substr(pos + 1));
                } else {
                    res.emplace(arg.substr(0, pos), "");
                }
            }
        }
        return res;
    };


}
