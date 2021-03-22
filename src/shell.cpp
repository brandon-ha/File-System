#include "shell.hpp"

// tokenize a string into a vector of strings
std::vector<std::string> tokenize(std::string str) {
    std::vector<std::string> result;
    std::stringstream ss(str);
    std::string current;

    while(ss >> current) {
        result.push_back(current);
    }

    return result;
}

// print str to cout
void Shell::print(std::string str) {
    if ((*out).is_open()) {
        *out << str << std::endl;
    } else {
        std::cout << str << std::endl;
    }
}

// shell constructors
Shell::Shell(std::ifstream& in_f, std::ofstream& out_f) {
    in = &in_f;
    out = &out_f;
}

// run shell
void Shell::run() {
    while (true) {
        try {
            int err_status = 0;
            std::string input;
            std::string command;

            if ((*in).is_open()) {
                if (!std::getline(*in, input)) {
                    break;
                }
            } else {
                std::getline(std::cin, input);
            }
            std::vector<std::string> tokenized = tokenize(input);

            if (tokenized.size() > 0) {
                command = tokenized[0];
            }

            if (command == "in") {
                
                err_status = fs.init();

                if (err_status == 0) {
                    print("system initialized");
                }

            } else if (command == "cr") {
                if (tokenized.size() == 2) {
                    std::string name = tokenized[1];
                    // splice name if necessary
                    name = name.substr(0, std::min(4, (int) name.size()));

                    err_status = fs.create(name);
                    
                    if (err_status == 0) {
                        std::string res = name + " created";
                        print(res);
                    }
                } else {
                    err_status = -1;
                }

            } else if (command == "de") {
                if (tokenized.size() == 2) {
                    std::string name = tokenized[1];
                    // splice name if necessary
                    name = name.substr(0, std::min(4, (int) name.size()));

                    err_status = fs.destroy(name);
                    
                    if (err_status == 0) {
                        std::string res = name + " destroyed";
                        print(res);
                    }
                } else {
                    err_status = -1;
                }

            } else if (command == "op") {
                if (tokenized.size() == 2) {
                    std::string name = tokenized[1];
                    // splice name if necessary
                    name = name.substr(0, std::min(4, (int) name.size()));

                    int oft_index = fs.open(name);
                    
                    if (oft_index >= 0) {
                        std::string res = name;
                        res += " opened ";
                        res += std::to_string(oft_index);
                        print(res);
                    } else {
                        err_status = -1;
                    }
                } else {
                    err_status = -1;
                }

            } else if (command == "cl") {
                if (tokenized.size() == 2) {
                    int oft_index = std::stoi(tokenized[1]);
                    
                    err_status = fs.close(oft_index);
                    
                    if (err_status == 0) {
                        std::string res = std::to_string(oft_index) + " closed";
                        print(res);
                    }
                } else {
                    err_status = -1;
                }

            } else if (command == "rd") {
                if (tokenized.size() == 4) {
                    int index = std::stoi(tokenized[1]);
                    int mem_pos = std::stoi(tokenized[2]);
                    int num_bytes = std::stoi(tokenized[3]);

                    int bytes_read = fs.read(index, mem_pos, num_bytes);

                    if (bytes_read >= 0) {
                        std::string res = std::to_string(bytes_read) + " bytes read from " + std::to_string(index);
                        print(res);
                    } else {
                        err_status = -1;
                    }
                } else {
                    err_status = -1;
                }

            } else if (command == "wr") {
                if (tokenized.size() == 4) {
                    int index = std::stoi(tokenized[1]);
                    int mem_pos = std::stoi(tokenized[2]);
                    int num_bytes = std::stoi(tokenized[3]);

                    int written = fs.write(index, mem_pos, num_bytes);

                    if (written >= 0) {
                        std::string res = std::to_string(written) + " bytes written to " + std::to_string(index);
                        print(res);
                    } else {
                        err_status = -1;
                    }
                } else {
                    err_status = -1;
                }
                
            } else if (command == "sk") {
                if (tokenized.size() == 3) {
                    int index = std::stoi(tokenized[1]);
                    int pos = std::stoi(tokenized[2]);

                    err_status = fs.seek(index, pos);

                    if (err_status == 0) {
                        std::string res = "position is " + std::to_string(pos);
                        print(res);
                    }
                } else {
                    err_status = -1;
                }
                
            } else if (command == "rm") {
                if (tokenized.size() == 3) {
                    int mem_pos = std::stoi(tokenized[1]);
                    int num_bytes = std::stoi(tokenized[2]);
                    std::pair<int, std::string> result;
                    
                    result = fs.read_memory(mem_pos, num_bytes);
                    err_status = result.first;

                    if (err_status == 0) {
                        print(result.second);
                    }
                } else {
                    err_status = -1;
                }

            } else if (command == "wm") {
                if (tokenized.size() >= 3) {
                    int mem_pos = std::stoi(tokenized[1]);

                    // join all words together if needed
                    std::string str = tokenized[2];
                    if (tokenized.size() > 3) {
                        for (int i = 3; i < (int) tokenized.size(); i++) {
                            str += " ";
                            str += tokenized[i];
                        }
                    }
                        
                    int bytes = fs.write_memory(mem_pos, str);

                    if (bytes > 0) {
                        std::string res = std::to_string(bytes) + " bytes written to M";
                        print(res);
                    } else {
                        err_status = -1;
                    }
                } else {
                    err_status = -1;
                }

            } else if (command == "dr") {
                print(fs.list_directory());

            } else if (tokenized.size() == 0) {
                print("");
                
            } else {
                err_status = -1;
            }

            if (err_status) {
                print("error");
            }
        } catch (int e) {
            print("error");
        }
    }
}