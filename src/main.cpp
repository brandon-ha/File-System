#include <fstream>

#include "shell.hpp"

int main(int argc, char **argv) {
    // check for input and/or output file
    std::ifstream infile;
    std::ofstream outfile;

    if (argc == 2 || argc == 3) {
        infile.open(argv[1]);
        if (argc == 3) {
            outfile.open(argv[2]);
        }
    }

    Shell shell(infile, outfile);

    shell.run();

    if (infile.is_open()) {
        infile.close();
    }

    if (outfile.is_open()) {
        outfile.close();
    }

    return 0;
}