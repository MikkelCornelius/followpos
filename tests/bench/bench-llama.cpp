#include "bench.h"
#include <regex>

BenchResult bench_llama(const std::vector<std::string>& content, const std::string& pat) {
    using clock = std::chrono::steady_clock;
    BenchResult r;

    auto t0 = clock::now();
    std::regex reg(pat);
    std::string buffer;
    auto t1 = clock::now();
    r.init_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();

    auto t2 = clock::now();
    for (const std::string& tok : content) {
        ++r.tokens;
        buffer += tok;
        if (std::regex_search(buffer, reg)) {
            r.found = true;
            break;
        }
    }
    auto t3 = clock::now();
    r.match_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t3 - t2).count();
    return r;
}