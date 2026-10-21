#include <hs/hs.h>
#include "bench.h"

static int on_match(unsigned, unsigned long long, unsigned long long,
                    unsigned, void*) {
    return 1; // stop at the first match
}

BenchResult bench_vectorscan(const std::vector<std::string>& content, const std::string& pat) {
    using clock = std::chrono::steady_clock;
    BenchResult r;

    auto t0 = clock::now();
    hs_database_t* db = nullptr;
    hs_compile_error_t* err = nullptr;
    if (hs_compile(pat.c_str(), 0, HS_MODE_STREAM, nullptr, &db, &err) != HS_SUCCESS) {
        hs_free_compile_error(err);
        return r;
    }
    hs_scratch_t* scratch = nullptr;
    hs_alloc_scratch(db, &scratch);
    hs_stream_t* stream = nullptr;
    hs_open_stream(db, 0, &stream);
    auto t1 = clock::now();
    r.init_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();

    auto t2 = clock::now();
    for (const std::string& tok : content) {
        ++r.tokens;
        hs_error_t rc = hs_scan_stream(stream, tok.data(), tok.size(), 0,
                                       scratch, on_match, nullptr);
        if (rc == HS_SCAN_TERMINATED) {
            r.found = true;
            break;
        }
    }
    auto t3 = clock::now();
    r.match_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t3 - t2).count();

    hs_close_stream(stream, scratch, nullptr, nullptr);
    hs_free_scratch(scratch);
    hs_free_database(db);
    return r;
}