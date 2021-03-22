#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>


#include "fs.hpp"

#pragma once

std::vector<std::string> tokenize(std::string str);

void print(std::string str);

class Shell {
    private:
        FileSystem fs;
        std::ifstream* in;
        std::ofstream* out;
        void print(std::string);

    public:
        Shell(std::ifstream& in, std::ofstream& out);
        void run();
};