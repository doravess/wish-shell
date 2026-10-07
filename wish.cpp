#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <unistd.h>
#include <sys/wait.h>
#include <cstring>
#include <fcntl.h>

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
    if (tokens.empty()) return false;

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

std::vector<std::string> split_by_delim(const std::string &s, char delim) {
    std::vector<std::string> result;
    std::string current;
    for (char ch : s) {
        if (ch == delim) {
            result.push_back(current);
            current.clear();
        } else {
            current += ch;
        }
    }
    result.push_back(current);
    return result;
}

std::vector<std::string> tokenize(const std::string &s) {
    std::stringstream ss(s);
    std::string token;
    std::vector<std::string> tokens;
    while (ss >> token) {
        tokens.push_back(token);
    }
    return tokens;
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

        std::vector<std::string> raw_cmds = split_by_delim(line, '&');
        std::vector<pid_t> pids;

        for (const auto &raw_cmd : raw_cmds) {
            size_t redirect_count = 0;
            for (char ch : raw_cmd) {
                if (ch == '>') redirect_count++;
            }

            if (redirect_count > 1) {
                print_error();
                continue;
            }

            std::string cmd_part = raw_cmd;
            std::string output_file = "";

            if (redirect_count == 1) {
                std::vector<std::string> parts = split_by_delim(raw_cmd, '>');
                cmd_part = parts[0];
                std::vector<std::string> file_tokens = tokenize(parts[1]);

                if (file_tokens.size() != 1) {
                    print_error();
                    continue;
                }
                output_file = file_tokens[0];
            }

            std::vector<std::string> tokens = tokenize(cmd_part);
            if (tokens.empty()) {
                if (redirect_count == 1) {
                    print_error();
                }
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
                if (!output_file.empty()) {
                    int fd = open(output_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    if (fd < 0) {
                        print_error();
                        _exit(1);
                    }
                    dup2(fd, STDOUT_FILENO);
                    dup2(fd, STDERR_FILENO);
                    close(fd);
                }
                execv(executable.c_str(), args.data());
                print_error();
                _exit(1);
            } else {
                pids.push_back(pid);
            }
        }

        for (pid_t p : pids) {
            waitpid(p, nullptr, 0);
        }
    }

    return 0;
}
