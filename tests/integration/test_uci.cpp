#include "doctest/doctest.h"
#include <optional>
#include <string>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

class StargazeProcess {
    pid_t pid = -1;
    int write_fd = -1;
    int read_fd = -1;

  public:
    StargazeProcess() {
        int parent_to_child[2];
        int child_to_parent[2];

        if (pipe(parent_to_child) != 0 || pipe(child_to_parent) != 0) {
            FAIL("Failed to create pipes");
        }

        pid = fork();
        if (pid < 0) {
            FAIL("Failed to fork process");
        }

        if (pid == 0) {
            // Child process
            dup2(parent_to_child[0], STDIN_FILENO);
            dup2(child_to_parent[1], STDOUT_FILENO);

            close(parent_to_child[0]);
            close(parent_to_child[1]);
            close(child_to_parent[0]);
            close(child_to_parent[1]);

            execl("./bin/stargaze", "./bin/stargaze", nullptr);
            perror("execl failed");
            exit(1);
        } else {
            // Parent process
            close(parent_to_child[0]);
            close(child_to_parent[1]);

            write_fd = parent_to_child[1];
            read_fd = child_to_parent[0];
        }
    }

    ~StargazeProcess() {
        if (write_fd != -1)
            close(write_fd);
        if (read_fd != -1)
            close(read_fd);
        if (pid != -1) {
            int status;
            waitpid(pid, &status, WNOHANG);
        }
    }

    void write_line(const std::string &line) {
        std::string full_line = line + "\n";
        ssize_t bytes_written =
            write(write_fd, full_line.c_str(), full_line.size());
        REQUIRE(bytes_written == static_cast<ssize_t>(full_line.size()));
    }

    std::optional<std::string> read_line() {
        std::string line;
        char c;
        bool has_chars = false;
        while (true) {
            ssize_t bytes_read = read(read_fd, &c, 1);
            if (bytes_read <= 0) {
                if (!has_chars)
                    return std::nullopt;
                break;
            }
            has_chars = true;
            if (c == '\n')
                break;
            line += c;
        }
        return line;
    }

    std::string read_until(const std::string &target,
                           std::vector<std::string> &log) {
        while (true) {
            auto opt_line = read_line();
            if (!opt_line) {
                FAIL("Child process terminated prematurely (EOF reached)");
            }
            std::string line = *opt_line;
            log.push_back(line);
            if (line.find(target) != std::string::npos) {
                return line;
            }
        }
    }
};

TEST_SUITE("integration") {
    TEST_CASE("UCI Basic Protocol Interaction") {
        StargazeProcess proc;
        std::vector<std::string> log;

        proc.write_line("uci");
        proc.read_until("uciok", log);
        bool found_name = false;
        bool found_author = false;
        bool found_uciok = false;
        for (const auto &line : log) {
            if (line.find("id name stargaze") != std::string::npos)
                found_name = true;
            if (line.find("id author mkutay") != std::string::npos)
                found_author = true;
            if (line.find("uciok") != std::string::npos)
                found_uciok = true;
        }
        CHECK(found_name);
        CHECK(found_author);
        CHECK(found_uciok);

        proc.write_line("isready");
        log.clear();
        proc.read_until("readyok", log);
        bool found_readyok = false;
        for (const auto &line : log) {
            if (line.find("readyok") != std::string::npos)
                found_readyok = true;
        }
        CHECK(found_readyok);

        proc.write_line("position startpos");
        proc.write_line("go depth 3");
        log.clear();
        std::string bestmove_line = proc.read_until("bestmove", log);
        CHECK(bestmove_line.find("bestmove") != std::string::npos);

        proc.write_line("quit");
    }
}
