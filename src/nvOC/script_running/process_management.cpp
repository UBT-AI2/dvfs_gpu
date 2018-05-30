//
// Created by alex on 30.05.18.
//

#include "process_management.h"
#ifdef _MSC_VER
#include <windows.h>
#else
#endif

namespace frequency_scaling {

    static const int BUFFER_SIZE = 4096;

    FILE *process_management::popen_file(const std::string &cmd) {
        FILE* fp;
#ifdef _MSC_VER
        fp = _popen(cmd.c_str(), "r");
#else
        fp = popen(cmd.c_str(), "r");
#endif
        return fp;
    }


    std::string process_management::popen_string(const std::string &cmd) {
        FILE* fp = popen_file(cmd);
        if(fp == NULL)
            return "";

        std::string res;
        char line[BUFFER_SIZE];
        /* Read the output a line at a time - output it. */
        while (fgets(line, BUFFER_SIZE, fp) != NULL) {
            res += line;
        }
        fclose(fp);
        return res;
    }

}