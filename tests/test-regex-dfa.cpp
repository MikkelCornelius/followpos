#include "common.h"

#include <cstdio>
#include <stdexcept>
#include <iostream>

static int n_fail = 0;

#define CHECK(cond)                                                \
    do {                                                           \
        if (!(cond)) {                                             \
            printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); \
            n_fail++;                                              \
        }                                                          \
    } while (0)

using positions = std::set<int>;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static std::string to_string(const Node & n) {
    switch (n.type) {
        case REGEX_NODE_EPSILON:       return "e";
        case REGEX_NODE_SYMBOL:        return std::string(1, (char) n.value);
        case REGEX_NODE_CONCATENATION: return "(" + to_string(*n.left_child) + "." + to_string(*n.right_child) + ")";
        case REGEX_NODE_ALTERNATION:   return "(" + to_string(*n.left_child) + "|" + to_string(*n.right_child) + ")";
        case REGEX_NODE_REPETITION:    return "(" + to_string(*n.left_child) + "*)";
    }
    return "";
}

// Hand-built syntax tree nodes (all pointers initialized, so trees are safe to free)
static Node* leaf(int position, char32_t c) {
    auto n         = new Node;
    n->type        = REGEX_NODE_SYMBOL;
    n->value       = c;
    n->position    = position;
    n->left_child  = nullptr;
    n->right_child = nullptr;
    return n;
}

static Node* make(regex_node_type type, Node* left = nullptr, Node* right = nullptr) {
    auto n         = new Node;
    n->type        = type;
    n->left_child  = left;
    n->right_child = right;
    return n;
}

static Node* eps() { return make(REGEX_NODE_EPSILON); }
static Node* cat(Node* l, Node* r) { return make(REGEX_NODE_CONCATENATION, l, r); }
static Node* alt(Node* l, Node* r) { return make(REGEX_NODE_ALTERNATION, l, r); }
static Node* star(Node* c) { return make(REGEX_NODE_REPETITION, c); }

// Parse a pattern and compare the augmented tree and symbol table
static void check_parse(const std::string & pattern, const std::string & expected_tree, const std::vector<char32_t> & expected_symbols) {
    std::string           p = pattern;
    std::vector<char32_t> symbol_at;
    Node *                root = augment_pattern(regex_parse(p, &symbol_at), &symbol_at);

    std::string got = to_string(*root);
    if (got != expected_tree) {
        printf("FAIL parse '%s': tree is %s, expected %s\n", pattern.c_str(), got.c_str(), expected_tree.c_str());
        n_fail++;
    }
    if (symbol_at != expected_symbols) {
        printf("FAIL parse '%s': got %zu symbols, expected %zu\n", pattern.c_str(), symbol_at.size(), expected_symbols.size());
        n_fail++;
    }
}

struct analyzed {
    Node *                     root;
    std::vector<char32_t>      symbol_at;
    std::vector<std::set<int>> followpos;
};

// Parse, augment and annotate a pattern
static analyzed analyze(std::string pattern) {
    analyzed a;
    a.root = augment_pattern(regex_parse(pattern, &a.symbol_at), &a.symbol_at);
    a.followpos.resize(a.symbol_at.size());
    regex_annotate(a.root, &a.followpos);
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

// ---------------------------------------------------------------------------
// Parser tests (one per case, so one failure doesn't hide the others)
// ---------------------------------------------------------------------------

static void test_parse_empty()      { check_parse("",    "(e.#)",         { '#' }); }
static void test_parse_symbol()     { check_parse("a",   "(a.#)",         { 'a', '#' }); }
static void test_parse_concat()     { check_parse("ab",  "((a.b).#)",     { 'a', 'b', '#' }); }
static void test_parse_repeat_sym() { check_parse("aa",  "((a.a).#)",     { 'a', 'a', '#' }); }
static void test_parse_alt()        { check_parse("a|b", "((a|b).#)",     { 'a', 'b', '#' }); }
static void test_parse_cat_alt()    { check_parse("aa|b", "(a.(a|b).#)",     { 'a', 'a', 'b', '#' }); }
static void test_parse_star()       { check_parse("a*",  "((a*).#)",      { 'a', '#' }); }
static void test_parse_cat_star()   { check_parse("ab*", "((a.(b*)).#)",  { 'a', 'b', '#' }); }
static void test_parse_precedence() { check_parse("aa|ab*", "(((a.a)|(a.(b*))).#)", { 'a', 'a', 'a', 'b', '#' }); }
static void test_parse_group()      { check_parse("(a|b)*", "(((a|b)*).#)", { 'a', 'b', '#' }); }
static void test_parse_group_cat()  { check_parse("(ab)(ab)", "(((a.b).(a.b)).#)", { 'a', 'b', 'a', 'b', '#' }); }
static void test_parse_full()       { check_parse("(a|b)*abb", "((((((a|b)*).a).b).b).#)", { 'a', 'b', 'a', 'b', 'b', '#' }); }

// ---------------------------------------------------------------------------
// Annotation tests on hand-built trees (independent of the parser)
// ---------------------------------------------------------------------------

static void test_annotate_epsilon() {  // (e)#
    auto root = cat(eps(), leaf(0, '#'));
    std::vector<std::set<int>> followpos(1);

    regex_annotate(root, &followpos);

    CHECK(root->left_child->nullable);
    CHECK(root->left_child->firstpos.empty());
    CHECK(root->left_child->lastpos.empty());
    CHECK(!root->nullable);
    CHECK(root->firstpos == positions({ 0 }));
    CHECK(root->lastpos == positions({ 0 }));
    CHECK(followpos[0].empty());
    free_tree(root);
}

static void test_annotate_alternation() {  // (a|b)#
    auto root = cat(alt(leaf(0, 'a'), leaf(1, 'b')), leaf(2, '#'));
    std::vector<std::set<int>> followpos(3);

    regex_annotate(root, &followpos);

    CHECK(!root->nullable);
    CHECK(root->firstpos == positions({ 0, 1 }));
    CHECK(root->lastpos == positions({ 2 }));
    CHECK(followpos[0] == positions({ 2 }));
    CHECK(followpos[1] == positions({ 2 }));
    CHECK(followpos[2].empty());
    free_tree(root);
}

static void test_annotate_concat_repetition() {  // (ab*)#
    auto root = cat(
        cat(leaf(0, 'a'), star(leaf(1, 'b'))),
        leaf(2, '#'));
    std::vector<std::set<int>> followpos(3);

    regex_annotate(root, &followpos);

    CHECK(!root->nullable);
    CHECK(root->firstpos == positions({ 0 }));
    CHECK(root->lastpos == positions({ 2 }));
    CHECK(followpos[0] == positions({ 1, 2 }));
    CHECK(followpos[1] == positions({ 1, 2 }));
    CHECK(followpos[2].empty());
    free_tree(root);
}

static void test_annotate_star_of_alternation() {  // ((a|b)*)#
    auto root = cat(star(alt(leaf(0, 'a'), leaf(1, 'b'))), leaf(2, '#'));
    std::vector<std::set<int>> followpos(3);

    regex_annotate(root, &followpos);

    CHECK(!root->nullable);
    CHECK(root->left_child->nullable);
    CHECK(root->firstpos == positions({ 0, 1, 2 }));
    CHECK(root->lastpos == positions({ 2 }));
    CHECK(followpos[0] == positions({ 0, 1, 2 }));
    CHECK(followpos[1] == positions({ 0, 1, 2 }));
    CHECK(followpos[2].empty());
    free_tree(root);
}

// ---------------------------------------------------------------------------
// End-to-end: parse + annotate
// ---------------------------------------------------------------------------

static void test_empty() {
    analyzed a = analyze("");  // parses as (epsilon)#
    CHECK(!a.root->nullable);
    CHECK(a.root->firstpos == positions({ 0 }));
    CHECK(a.root->lastpos == positions({ 0 }));
    CHECK(a.followpos[0].empty());
}

static void test_symbol() {  // a#
    analyzed a = analyze("a");
    CHECK(!a.root->nullable);
    CHECK(a.root->firstpos == positions({ 0 }));
    CHECK(a.root->lastpos == positions({ 1 }));
    CHECK(a.followpos[0] == positions({ 1 }));
    CHECK(a.followpos[1].empty());
}

static void test_alternation() {  // (a|b)#
    analyzed a = analyze("a|b");
    CHECK(!a.root->nullable);
    CHECK(a.root->firstpos == positions({ 0, 1 }));
    CHECK(a.root->lastpos == positions({ 2 }));
    CHECK(a.followpos[0] == positions({ 2 }));
    CHECK(a.followpos[1] == positions({ 2 }));
    CHECK(a.followpos[2].empty());
}

static void test_concatenation() {  // (ab)#
    analyzed a = analyze("ab");
    CHECK(!a.root->nullable);
    CHECK(a.root->firstpos == positions({ 0 }));
    CHECK(a.root->lastpos == positions({ 2 }));
    CHECK(a.followpos[0] == positions({ 1 }));
    CHECK(a.followpos[1] == positions({ 2 }));
    CHECK(a.followpos[2].empty());
}

static void test_repetition() {  // (a*)#
    analyzed a = analyze("a*");
    CHECK(!a.root->nullable);
    CHECK(a.root->firstpos == positions({ 0, 1 }));
    CHECK(a.root->lastpos == positions({ 1 }));
    CHECK(a.followpos[0] == positions({ 0, 1 }));
    CHECK(a.followpos[1].empty());
}

static void test_concat_repetition() {  // (ab*)#
    analyzed a = analyze("ab*");
    CHECK(!a.root->nullable);
    CHECK(a.root->firstpos == positions({ 0 }));
    CHECK(a.root->lastpos == positions({ 2 }));
    CHECK(a.followpos[0] == positions({ 1, 2 }));
    CHECK(a.followpos[1] == positions({ 1, 2 }));
    CHECK(a.followpos[2].empty());
}

int main() {
    run("parse_empty", test_parse_empty);
    run("parse_symbol", test_parse_symbol);
    run("parse_concat", test_parse_concat);
    run("parse_repeat_sym", test_parse_repeat_sym);
    run("parse_alt", test_parse_alt);
    run("parse_cat_alt", test_parse_cat_alt);
    run("parse_star", test_parse_star);
    run("parse_cat_star", test_parse_cat_star);
    run("parse_precedence", test_parse_precedence);
    run("parse_group", test_parse_group);
    run("parse_group_cat", test_parse_group_cat);
    run("parse_full", test_parse_full);

    run("annotate_epsilon", test_annotate_epsilon);
    run("annotate_alternation", test_annotate_alternation);
    run("annotate_concat_repetition", test_annotate_concat_repetition);
    run("annotate_star_of_alternation", test_annotate_star_of_alternation);

    run("empty", test_empty);
    run("symbol", test_symbol);
    run("alternation", test_alternation);
    run("concatenation", test_concatenation);
    run("repetition", test_repetition);
    run("concat_repetition", test_concat_repetition);

    return n_fail == 0 ? 0 : 1;
}