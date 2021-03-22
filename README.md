# File System

This project implements a file system that utilizes an emulated disk. By typing in specific commands directly in the shell or by feeding in input files, you can control the contents of the file system and memory buffer.

# User Interface

| Command                | Description                                                                   |
|------------------------|-------------------------------------------------------------------------------|
| cr {file-name}         | Create file with filename {file-name}                                         |
| de {file-name}         | Delete file with filename {file-name}                                         |
| op {file-name}         | Open file with filename {file-name}                                           |
| cl {oft-index}         | Close file with corresponding open file table index                           |
| rd {oft-index} {m} {n} | Read n bytes file with corresponding index into memory starting at location m |
| sk {oft-index} {p}     | Seek position p for corresponding open file                                   |
| dr                     | List files in directory                                                       |
| in                     | (Re-)/Initialize file system                                                  |
| rm {m} {n}             | Read n bytes from memory starting at location m                               |
| wm {m} {s}             | Write string s into memory starting at location m                             |

# Compilation

To compile, just use:

make

OR

g++ -Wall -std=c++11 -Iinclude -o fs src/*.cpp

# Running program

To run the program, you can use the following to input from command line directly:

./fs

If you want to use an input file, you may enter the input file name as the second argument.

./fs {input file}

If you want to output to a file, you may enter the output file name as the the third argument (must be used with input file).

./fs {input file} {output file}

## Example execution

./fs input.txt output.txt
