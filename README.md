# HOW TO COMPILE

To compile, just use:

make

OR

g++ -Wall -std=c++11 -Iinclude -o fs src/*.cpp

# USAGE

To run the program, you can use the following to input from command line directly:

./fs

If you want to use an input file, you may enter the input file name as the second argument.

./fs {input file}

If you want to output to a file, you may enter the output file name as the the third argument (must be used with input file).

./fs {input file} {output file}

# EXAMPLE

./fs input.txt output.txt