#include "bench.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

std::vector<std::string> read_tokens(const std::string & path) {
    std::vector<std::string> tokens;
    std::ifstream            file(path);
    std::string              tok;
    char                     comma;
    while (file >> std::quoted(tok)) {
        tokens.push_back(tok);
        file >> comma;
    }
    return tokens;
}

static void report(const char * name, const BenchResult & r) {
    std::cout << name << ": init " << r.init_us << "us, " << r.match_ns / (long) r.tokens << "ns/token" << std::endl;
}

struct BenchmarkCase {
    const char * file;
    const char * pattern;
};

static const std::vector<BenchmarkCase> cases = {
    // Natural language
    { "english", "<tool_call>" },
    { "danish", "<funktionskald>" },
    { "german", "<funktionsaufruf>" },
    { "spanish", "<llamada_herramienta>" },
    { "greek", "<κλήση_εργαλείου>" },
    { "japanese", "<ツール呼び出し>" },
    { "english_regex", "<tool_call>\\s*\\{\\s*\"name\"" },
    { "english_short", "<tool_call>" },
    // Formal language
    { "python", "(eval|exec)\\s*\\(" },
    { "https", "(%3[Cc]|<)[sS][cC][rR][iI][pP][tT](%3[Ee]|>)" },
    // Custom language
    { "long",
      "(abcd|abce|abde|acde|bcde)(a|bc|de)(abc|abd|abe|acd|ace|ade)(bcd|bce|bde|cde)("
      "aab|aac|aad|aae|abb|abc|abd|abe)(de|ea|ab|bc)(abcde|abcda|abcdb|abcdc|abcde)" },
    { "backtrack", "(a|aa)+b" },
    { "state_explosion", "(a|b)*a(a|b){7}" },
    { "alphabet_test", "(start[A-z]{4}middle|begin[A-z]{4}end)finish" },
    { "alphabet_test_japanese", "(start[一-鿿]{4}middle|begin[一-鿿]{4}end)finish" },
    { "match_any_repeated", "(a|b)*bbb" },
};

static int run_worker(size_t index, const std::string & engine) {
    const BenchmarkCase & test = cases[index];
    std::vector<std::string> content = read_tokens(std::string("tests/bench/content/") + test.file + ".txt");

    if (engine == "followpos") {
        report("followpos ", bench_followpos(content, test.pattern));
    } else if (engine == "llama") {
        report("llama.cpp ", bench_llama(content, test.pattern));
    } else if (engine == "vectorscan") {
        report("vectorscan", bench_vectorscan(content, test.pattern));
    } else {
        std::cerr << "Unknown benchmark engine: " << engine << std::endl;
        return 2;
    }
    return 0;
}

static bool run_isolated(char * executable, size_t index, const char * engine) {
    const pid_t pid = fork();
    if (pid == -1) {
        std::cerr << "Failed to fork benchmark worker: " << std::strerror(errno) << std::endl;
        return false;
    }
    if (pid == 0) {
        std::string index_arg = std::to_string(index);
        std::string engine_arg(engine);
        char        worker_option[] = "--worker";
        char * const args[]          = { executable, worker_option, index_arg.data(), engine_arg.data(), nullptr };
        execvp(executable, args);
        std::cerr << "Failed to start benchmark worker: " << std::strerror(errno) << std::endl;
        _exit(127);
    }

    int status;
    while (waitpid(pid, &status, 0) == -1) {
        if (errno != EINTR) {
            std::cerr << "Failed to wait for benchmark worker: " << std::strerror(errno) << std::endl;
            return false;
        }
    }
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        std::cerr << "Benchmark worker failed for " << engine << " on " << cases[index].file << std::endl;
        return false;
    }
    return true;
}

int main(int argc, char ** argv) {
    if (argc == 4 && std::string(argv[1]) == "--worker") {
        char * end = nullptr;
        errno      = 0;
        const unsigned long index = std::strtoul(argv[2], &end, 10);
        if (errno != 0 || end == argv[2] || *end != '\0' || index >= cases.size()) {
            std::cerr << "Invalid benchmark case index" << std::endl;
            return 2;
        }
        return run_worker(index, argv[3]);
    }

    for (size_t index = 0; index < cases.size(); ++index) {
        std::cout << "\n" << cases[index].file << std::endl;
        const char * engines[] = { "followpos", "llama", "vectorscan" };
        for (const char * engine : engines) {
            if (std::string(cases[index].file) == "backtrack" && std::string(engine) == "llama") {
                continue;  // this case breaks llama.cpp due to exponential complexity
            }
            if (!run_isolated(argv[0], index, engine)) {
                return 1;
            }
        }
    }
}
