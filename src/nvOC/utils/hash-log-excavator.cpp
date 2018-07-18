//
// Created by Alex on 09.07.2018.
//

#include <fstream>
#include <iostream>
#include <sstream>
#include <chrono>
#include <string>
#include <regex>
#include <vector>

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cout << "Usage: <excavator-binary> | " << argv[0] << " <filename>" << std::endl;
        return 1;
    }
    std::ofstream logfile(argv[1], std::ofstream::app);
    if (!logfile) {
        std::cout << "Failed to create logfile" << std::endl;
        return 1;
    }

    std::string lineInput;
    while (std::getline(std::cin, lineInput)) {
        std::cout << lineInput << std::endl;
        if (!std::regex_match(lineInput, std::regex(".*[-+]?[0-9]*\\.?[0-9]+([eE][-+]?[0-9]+)? H/s")))
            continue;
        //get system time
        long long int system_time = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
        //get hashrate
        std::istringstream iss(lineInput);
        std::vector<std::string> tokens{std::istream_iterator<std::string>{iss},
                                        std::istream_iterator<std::string>{}};
        double hashrate = std::stod(tokens[2]);
        //log to file
        logfile << system_time << " " << hashrate << std::endl;
    }
    return 0;
}

