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

using positions = std::set<size_t>;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static std::string to_string(const Node & n) {
    switch (n.type) {
        case REGEX_NODE_EPSILON:
            return "e";
        case REGEX_NODE_SYMBOL:
            return std::string(1, (char) n.value);
        case REGEX_NODE_CONCATENATION:
            return "(" + to_string(*n.left_child) + "." + to_string(*n.right_child) + ")";
        case REGEX_NODE_ALTERNATION:
            return "(" + to_string(*n.left_child) + "|" + to_string(*n.right_child) + ")";
        case REGEX_NODE_REPETITION:
            return "(" + to_string(*n.left_child) + "*)";
    }
    return "";
}

// Hand-built syntax tree nodes (all pointers initialized, so trees are safe to free)
static Node * leaf(int position, char32_t c) {
    auto n         = new Node;
    n->type        = REGEX_NODE_SYMBOL;
    n->value       = c;
    n->position    = position;
    n->left_child  = nullptr;
    n->right_child = nullptr;
    return n;
}

static Node * make(regex_node_type type, Node * left = nullptr, Node * right = nullptr) {
    auto n         = new Node;
    n->type        = type;
    n->left_child  = left;
    n->right_child = right;
    return n;
}

static Node * eps() {
    return make(REGEX_NODE_EPSILON);
}

static Node * cat(Node * l, Node * r) {
    return make(REGEX_NODE_CONCATENATION, l, r);
}

static Node * alt(Node * l, Node * r) {
    return make(REGEX_NODE_ALTERNATION, l, r);
}

static Node * star(Node * c) {
    return make(REGEX_NODE_REPETITION, c);
}

// Parse a pattern and compare the augmented tree and symbol table
static void check_parse(const std::string &           pattern,
                        const std::string &           expected_tree,
                        const std::vector<char32_t> & expected_symbols) {
    std::vector<char32_t> symbol_at;
    Parser                p{ pattern, symbol_at };
    Node *                tree = p.parse();
    Node *                root = augment_pattern(tree, symbol_at);

    std::string got = to_string(*root);
    if (got != expected_tree) {
        printf("FAIL parse '%s': tree is %s, expected %s\n", pattern.c_str(), got.c_str(), expected_tree.c_str());
        n_fail++;
    }
    if (symbol_at != expected_symbols) {
        printf("FAIL parse '%s': got %zu symbols, expected %zu\n", pattern.c_str(), symbol_at.size(),
               expected_symbols.size());
        n_fail++;
    }
}

struct analyzed {
    std::set<positions> dstates;
    Transitions         dtran;
};

// Build states and transitions for a pattern
static analyzed analyze(std::string pattern) {
    analyzed                   a;
    DFA                        dfa;
    std::set<size_t>           initial_state_positions;
    std::set<std::set<size_t>> accepting_state_positions;
    followpos(pattern, dfa.alphabet, initial_state_positions, accepting_state_positions, a.dstates, a.dtran);
    return a;
}

static bool has_transition(const analyzed & a, const positions & from, char32_t symbol, const positions & to) {
    auto state = a.dtran.find(from);
    if (state == a.dtran.end()) {
        return false;
    }
    for (const auto & transition : state->second) {
        if (transition.symbol == symbol && transition.to == to) {
            return true;
        }
    }
    return false;
}

static size_t packed_transition(const DFA & dfa, size_t state, char32_t symbol) {
    if (state >= dfa.base.size()) {
        return 0;
    }
    size_t index = dfa.base[state] + symbol;
    if (index >= dfa.check.size() || dfa.check[index] != state) {
        return 0;
    }
    return dfa.next_state[index];
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

static void test_parse_empty() {
    check_parse("", "(e.#)", { '#' });
}

static void test_parse_symbol() {
    check_parse("a", "(a.#)", { 'a', '#' });
}

static void test_parse_concat() {
    check_parse("ab", "((a.b).#)", { 'a', 'b', '#' });
}

static void test_parse_repeat_sym() {
    check_parse("aa", "((a.a).#)", { 'a', 'a', '#' });
}

static void test_parse_alt() {
    check_parse("a|b", "((a|b).#)", { 'a', 'b', '#' });
}

static void test_parse_cat_alt() {
    check_parse("aa|b", "(((a.a)|b).#)", { 'a', 'a', 'b', '#' });
}

static void test_parse_star() {
    check_parse("a*", "((a*).#)", { 'a', '#' });
}

static void test_parse_cat_star() {
    check_parse("ab*", "((a.(b*)).#)", { 'a', 'b', '#' });
}

static void test_parse_precedence() {
    check_parse("aa|ab*", "(((a.a)|(a.(b*))).#)", { 'a', 'a', 'a', 'b', '#' });
}

static void test_parse_group() {
    check_parse("(a|b)*", "(((a|b)*).#)", { 'a', 'b', '#' });
}

static void test_parse_group_cat() {
    check_parse("(ab)(ab)", "(((a.b).(a.b)).#)", { 'a', 'b', 'a', 'b', '#' });
}

static void test_parse_full() {
    check_parse("(a|b)*abb", "((((((a|b)*).a).b).b).#)", { 'a', 'b', 'a', 'b', 'b', '#' });
}

// ---------------------------------------------------------------------------
// Annotation tests on hand-built trees (independent of the parser)
// ---------------------------------------------------------------------------

static void test_annotate_epsilon() {  // (e)#
    auto                          root = cat(eps(), leaf(0, '#'));
    std::vector<std::set<size_t>> followpos(1);

    regex_annotate(*root, followpos);

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
    auto                          root = cat(alt(leaf(0, 'a'), leaf(1, 'b')), leaf(2, '#'));
    std::vector<std::set<size_t>> followpos(3);

    regex_annotate(*root, followpos);

    CHECK(!root->nullable);
    CHECK(root->firstpos == positions({ 0, 1 }));
    CHECK(root->lastpos == positions({ 2 }));
    CHECK(followpos[0] == positions({ 2 }));
    CHECK(followpos[1] == positions({ 2 }));
    CHECK(followpos[2].empty());
    free_tree(root);
}

static void test_annotate_concat_repetition() {  // (ab*)#
    auto                          root = cat(cat(leaf(0, 'a'), star(leaf(1, 'b'))), leaf(2, '#'));
    std::vector<std::set<size_t>> followpos(3);

    regex_annotate(*root, followpos);

    CHECK(!root->nullable);
    CHECK(root->firstpos == positions({ 0 }));
    CHECK(root->lastpos == positions({ 2 }));
    CHECK(followpos[0] == positions({ 1, 2 }));
    CHECK(followpos[1] == positions({ 1, 2 }));
    CHECK(followpos[2].empty());
    free_tree(root);
}

static void test_annotate_star_of_alternation() {  // ((a|b)*)#
    auto                          root = cat(star(alt(leaf(0, 'a'), leaf(1, 'b'))), leaf(2, '#'));
    std::vector<std::set<size_t>> followpos(3);

    regex_annotate(*root, followpos);

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
// End-to-end: parse + followpos state construction
// ---------------------------------------------------------------------------

static void test_empty() {
    analyzed a = analyze("");
    CHECK(a.dstates == std::set<positions>({ positions({ 0 }) }));
    CHECK(a.dtran.at(positions({ 0 })).empty());
}

static void test_symbol() {  // a#
    analyzed a = analyze("a");
    CHECK(a.dstates == std::set<positions>({ positions({ 0 }), positions({ 1 }) }));
    CHECK(has_transition(a, positions({ 0 }), 'a', positions({ 1 })));
}

static void test_alternation() {  // (a|b)#
    analyzed a = analyze("a|b");
    CHECK(a.dstates == std::set<positions>({ positions({ 0, 1 }), positions({ 2 }) }));
    CHECK(has_transition(a, positions({ 0, 1 }), 'a', positions({ 2 })));
    CHECK(has_transition(a, positions({ 0, 1 }), 'b', positions({ 2 })));
}

static void test_concatenation() {  // (ab)#
    analyzed a = analyze("ab");
    CHECK(a.dstates == std::set<positions>({ positions({ 0 }), positions({ 1 }), positions({ 2 }) }));
    CHECK(has_transition(a, positions({ 0 }), 'a', positions({ 1 })));
    CHECK(has_transition(a, positions({ 1 }), 'b', positions({ 2 })));
}

static void test_repetition() {  // (a*)#
    analyzed a = analyze("a*");
    CHECK(a.dstates == std::set<positions>({ positions({ 0, 1 }) }));
    CHECK(has_transition(a, positions({ 0, 1 }), 'a', positions({ 0, 1 })));
}

static void test_concat_repetition() {  // (ab*)#
    analyzed a = analyze("ab*");
    CHECK(a.dstates == std::set<positions>({ positions({ 0 }), positions({ 1, 2 }) }));
    CHECK(has_transition(a, positions({ 0 }), 'a', positions({ 1, 2 })));
    CHECK(has_transition(a, positions({ 1, 2 }), 'b', positions({ 1, 2 })));
}

static void test_build_transition_table() {
    std::set<positions> dstates = { positions({ 1 }), positions({ 2 }), positions({ 3 }), positions({ 4 }) };
    Transitions         dtran;
    dtran[positions({
        1
    })]               = { { positions({ 2 }), 'a' }, { positions({ 3 }), 'b' }, { positions({ 4 }), 'c' } };
    dtran[positions({
        2
    })]               = { { positions({ 3 }), 'a' }, { positions({ 4 }), 'b' } };
    dtran[positions({
        3
    })]               = { { positions({ 1 }), 'c' } };
    dtran[positions({ 4 })] = {};

    DFA                       dfa;
    const positions           initial_state_positions = { 1 };
    const std::set<positions> accepting_state_positions;
    build_transition_table(dstates, dtran, dfa, initial_state_positions, accepting_state_positions);

    CHECK(dfa.alphabet.empty());
    CHECK(dfa.base.size() == 5);
    CHECK(packed_transition(dfa, 1, 'a') == 2);
    CHECK(packed_transition(dfa, 1, 'b') == 3);
    CHECK(packed_transition(dfa, 1, 'c') == 4);
    CHECK(packed_transition(dfa, 2, 'a') == 3);
    CHECK(packed_transition(dfa, 2, 'b') == 4);
    CHECK(packed_transition(dfa, 3, 'c') == 1);
    CHECK(packed_transition(dfa, 4, 'a') == 0);
    CHECK(dfa.base[1] != dfa.base[2]);
}

static void test_regex_to_dfa_alternation_concat() {
    DFA dfa = regex_to_dfa("a|bc");

    CHECK(dfa.alphabet == std::set<char32_t>({ '#', 'a', 'b', 'c' }));
    CHECK(dfa.base.size() == 4);
    CHECK(packed_transition(dfa, 1, 'a') == 3);
    CHECK(packed_transition(dfa, 1, 'b') == 2);
    CHECK(packed_transition(dfa, 2, 'c') == 3);
    CHECK(packed_transition(dfa, 1, 'c') == 0);
    CHECK(packed_transition(dfa, 2, 'a') == 0);
}

static void test_regex_to_dfa_repetition() {
    DFA dfa = regex_to_dfa("a*");

    CHECK(dfa.alphabet == std::set<char32_t>({ '#', 'a' }));
    CHECK(dfa.base.size() == 2);
    CHECK(packed_transition(dfa, 1, 'a') == 1);
    CHECK(packed_transition(dfa, 1, 'b') == 0);
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
    run("build_transition_table", test_build_transition_table);
    run("regex_to_dfa_alternation_concat", test_regex_to_dfa_alternation_concat);
    run("regex_to_dfa_repetition", test_regex_to_dfa_repetition);

    return n_fail == 0 ? 0 : 1;
}
