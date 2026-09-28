#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <unistd.h>
#include <sys/wait.h>
#include <cstring>

std::vector<std::string> paths;

void print_error() {
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
}

void init_paths() {
    paths.clear();
    paths.push_back("/bin");
}

bool handle_builtin(const std::vector<std::string> &tokens) {
    if (tokens[0] == "exit") {
        if (tokens.size() != 1) {
            print_error();
        } else {
            exit(0);
        }
        return true;
    } else if (tokens[0] == "cd") {
        if (tokens.size() != 2) {
            print_error();
        } else {
            if (chdir(tokens[1].c_str()) != 0) {
                print_error();
            }
        }
        return true;
    } else if (tokens[0] == "path") {
        paths.clear();
        for (size_t i = 1; i < tokens.size(); ++i) {
            paths.push_back(tokens[i]);
        }
        return true;
    }
    return false;
}

int main(int argc, char *argv[]) {
    std::istream *input_stream = &std::cin;
    std::ifstream file_stream;
    bool is_interactive = true;

    if (argc == 2) {
        file_stream.open(argv[1]);
        if (!file_stream.is_open()) {
            print_error();
            return 1;
        }
        input_stream = &file_stream;
        is_interactive = false;
    } else if (argc > 2) {
        print_error();
        return 1;
    }

    init_paths();

    std::string line;
    while (true) {
        if (is_interactive) {
            std::cout << "wish> ";
            std::cout.flush();
        }

        if (!std::getline(*input_stream, line)) {
            break;
        }

        std::stringstream ss(line);
        std::string token;
        std::vector<std::string> tokens;

        while (ss >> token) {
            tokens.push_back(token);
        }

        if (tokens.empty()) {
            continue;
        }

        if (handle_builtin(tokens)) {
            continue;
        }

        std::string executable;
        bool found = false;

        for (const auto &p : paths) {
            std::string candidate = p + "/" + tokens[0];
            if (access(candidate.c_str(), X_OK) == 0) {
                executable = candidate;
                found = true;
                break;
            }
        }

        if (!found) {
            print_error();
            continue;
        }

        std::vector<char*> args;
        for (auto &t : tokens) {
            args.push_back(&t[0]);
        }
        args.push_back(nullptr);

        pid_t pid = fork();
        if (pid < 0) {
            print_error();
        } else if (pid == 0) {
            execv(executable.c_str(), args.data());
            print_error();
            _exit(1);
        } else {
            waitpid(pid, nullptr, 0);
        }
    }
    return 0;
}
