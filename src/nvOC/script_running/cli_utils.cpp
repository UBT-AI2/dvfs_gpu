//
// Created by Alex on 16.06.2018.
//
#include "cli_utils.h"
#include <limits>
#include <regex>
#include <glog/logging.h>
#include "../common_header/fullexpr_accum.h"


namespace frequency_scaling {

    std::string cli_get_string(const std::string &user_msg,
                               const std::string &regex_to_match) {
        fullexpr_accum<>(std::cout) << user_msg << std::endl;
        std::string str;
        while (std::cin >> str) {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            if (std::regex_match(str, std::regex(regex_to_match)))
                return str;
            fullexpr_accum<>(std::cout) << "Invalid input. Try again." << std::endl;
            fullexpr_accum<>(std::cout) << user_msg << std::endl;
        }
        return "";
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
            } else if (std::regex_match(arg, std::regex("-[\\w-]+"))) {
                if (i + 1 < argc && !std::regex_match(argv[i + 1], std::regex("-[\\w-]+(=[\\w-./]+)?")))
                    res.emplace(arg, argv[++i]);
                else
                    res.emplace(arg, "");
            } else {
                LOG(ERROR) << "Cmd parsing: Argument " <<
                           arg << " not recognized" << std::endl;
            }
        }
        return res;
    };


}
