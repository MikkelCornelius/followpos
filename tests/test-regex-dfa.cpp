#include "common.h"

#include <cstdio>
#include <stdexcept>

static int n_fail = 0;

#define CHECK(cond)                                                \
    do {                                                           \
        if (!(cond)) {                                             \
            printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); \
            n_fail++;                                              \
        }                                                          \
    } while (0)

using positions = std::set<int>;

struct analyzed {
    Node                       root;
    std::vector<char32_t>      symbol_at;
    std::vector<std::set<int>> followpos;
};

static analyzed analyze(std::string pattern) {
    analyzed a;
    a.root      = regex_parse(pattern, &a.symbol_at);
    a.followpos.resize(a.symbol_at.size());
    regex_annotate(&a.root, &a.followpos);
    return a;
}

// Run one test; an exception (such as "not implemented") counts as a failure
static void run(const char * name, void (*fn)()) {
    int before = n_fail;
    try {
        fn();
    } catch (const std::exception & e) {
        printf("FAIL %s: exception: %s\n", name, e.what());
        n_fail++;
    }
    printf("%s %s\n", n_fail == before ? "ok  " : "FAIL", name);
}

static void test_empty() {
    analyzed a = analyze("");  // parses as (epsilon)#
    CHECK(!a.root.nullable);
    CHECK(a.root.firstpos == positions({ 0 }));
    CHECK(a.root.lastpos == positions({ 0 }));
    CHECK(a.followpos[0].empty());
}

static void test_symbol() {  // a#
    analyzed a = analyze("a");
    CHECK(!a.root.nullable);
    CHECK(a.root.firstpos == positions({ 0 }));
    CHECK(a.root.lastpos == positions({ 1 }));
    CHECK(a.followpos[0] == positions({ 1 }));
    CHECK(a.followpos[1].empty());
}

static void test_alternation() {  // (a|b)#
    analyzed a = analyze("a|b");
    CHECK(!a.root.nullable);
    CHECK(a.root.firstpos == positions({ 0, 1 }));
    CHECK(a.root.lastpos == positions({ 2 }));
    CHECK(a.followpos[0] == positions({ 2 }));
    CHECK(a.followpos[1] == positions({ 2 }));
    CHECK(a.followpos[2].empty());
}

static void test_concatenation() {  // (ab)#
    analyzed a = analyze("ab");
    CHECK(!a.root.nullable);
    CHECK(a.root.firstpos == positions({ 0 }));
    CHECK(a.root.lastpos == positions({ 2 }));
    CHECK(a.followpos[0] == positions({ 1 }));
    CHECK(a.followpos[1] == positions({ 2 }));
    CHECK(a.followpos[2].empty());
}

static void test_repetition() {  // (a*)#
    analyzed a = analyze("a*");
    CHECK(!a.root.nullable);
    CHECK(a.root.firstpos == positions({ 0, 1 }));
    CHECK(a.root.lastpos == positions({ 1 }));
    CHECK(a.followpos[0] == positions({ 0, 1 }));
    CHECK(a.followpos[1].empty());
}

static void test_concat_repetition() {  // (ab*)#
    analyzed a = analyze("ab*");
    CHECK(!a.root.nullable);
    CHECK(a.root.firstpos == positions({ 0 }));
    CHECK(a.root.lastpos == positions({ 2 }));
    CHECK(a.followpos[0] == positions({ 1, 2 }));
    CHECK(a.followpos[1] == positions({ 1, 2 }));
    CHECK(a.followpos[2].empty());
}

struct match_case {
    const char * pattern;
    const char * text;
    bool         expected;
};

static void test_matching() {
    const match_case cases[] = {
        { "a",         "a",    true  },
        { "a",         "b",    false },
        { "a",         "",     false },
        { "a|b",       "b",    true  },
        { "ab*",       "abbb", true  },
        { "ab*",       "ba",   false },
        { "(a|b)*abb", "aabb", true  },
        { "(a|b)*abb", "abab", false },
    };

    for (const match_case & c : cases) {
        DFA dfa = regex_to_dfa(c.pattern);
        if (regex_dfa_match(dfa, c.text) != c.expected) {
            printf("FAIL pattern '%s' text '%s' expected %d\n", c.pattern, c.text, c.expected);
            n_fail++;
        }
    }
}

bool regex_dfa_match(const DFA & dfa, const std::string & text) {
    throw std::logic_error("feature not implemented");
}

int main() {
    run("empty", test_empty);
    run("symbol", test_symbol);
    run("alternation", test_alternation);
    run("concatenation", test_concatenation);
    run("repetition", test_repetition);
    run("concat_repetition", test_concat_repetition);
    run("matching", test_matching);

    return n_fail == 0 ? 0 : 1;
}