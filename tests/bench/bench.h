#pragma once

#include "regex-to-dfa.h"
#include <chrono>

struct BenchResult {
    long   init_us  = 0;
    long   match_ns = 0;
    size_t tokens   = 0;
    bool   found    = false;
};

BenchResult bench_followpos(const std::vector<std::string>& content, const std::string& pat);
BenchResult bench_llama(const std::vector<std::string>& content, const std::string& pat);
BenchResult bench_vectorscan(const std::vector<std::string>& content, const std::string& pat);