#include "bench.h"
#include "regex-to-dfa.h"

BenchResult bench_followpos(const std::vector<std::string> & content, const std::string & pat) {
    using clock = std::chrono::steady_clock;
    BenchResult r;

    auto              t0  = clock::now();
    regex_dfa::DFA    dfa = regex_dfa::regex_to_dfa(pat);
    std::list<size_t> live_paths;
    auto              t1 = clock::now();
    r.init_us            = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();

    auto t2 = clock::now();
    for (const std::string & tok : content) {
        ++r.tokens;
        if (regex_dfa::feed(tok, dfa, live_paths)) {
            r.found = true;
            break;
        }
    }
    auto t3    = clock::now();
    r.match_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t3 - t2).count();
    return r;
}
