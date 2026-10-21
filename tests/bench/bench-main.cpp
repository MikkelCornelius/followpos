#include "bench.h"

#include <fstream>
#include <iomanip>
#include <iostream>

std::vector<std::string> read_tokens(const std::string& path) {
    std::vector<std::string> tokens;
    std::ifstream file(path);
    std::string tok;
    char comma;
    while (file >> std::quoted(tok)) {
        tokens.push_back(tok);
        file >> comma;
    }
    return tokens;
}

static void report(const char* name, const BenchResult& r) {
    std::cout << name << ": init " << r.init_ns << "ns, "
              << r.match_ns / (long) r.tokens << "ns/token"
              << std::endl;
}

int main() {
    std::vector<std::string> content = read_tokens("examples/tokens.txt");
    std::string pat = "<tool_call>";

    report("followpos ", bench_followpos(content, pat));
    report("llama.cpp ", bench_llama(content, pat));
    report("vectorscan", bench_vectorscan(content, pat));
}