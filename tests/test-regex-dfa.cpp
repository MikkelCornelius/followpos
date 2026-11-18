#include "regex-to-dfa.h"

#include <cstdio>
#include <stdexcept>

using namespace regex_dfa;

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
        case EPSILON:
            return "e";
        case SYMBOL:
            return std::string(1, (char) n.value);
        case CONCATENATION:
            return "(" + to_string(*n.left_child) + "." + to_string(*n.right_child) + ")";
        case ALTERNATION:
            return "(" + to_string(*n.left_child) + "|" + to_string(*n.right_child) + ")";
        case REPETITION:
            return "(" + to_string(*n.left_child) + "*)";
    }
    return "";
}

// Hand-built syntax tree nodes (all pointers initialized, so trees are safe to free)
static Node * leaf(int position, u_char c) {
    auto n         = new Node;
    n->type        = SYMBOL;
    n->value       = c;
    n->position    = position;
    n->left_child  = nullptr;
    n->right_child = nullptr;
    return n;
}

static Node * make(node_type type, Node * left = nullptr, Node * right = nullptr) {
    auto n         = new Node;
    n->type        = type;
    n->left_child  = left;
    n->right_child = right;
    return n;
}

static Node * eps() {
    return make(EPSILON);
}

static Node * cat(Node * l, Node * r) {
    return make(CONCATENATION, l, r);
}

static Node * alt(Node * l, Node * r) {
    return make(ALTERNATION, l, r);
}

static Node * star(Node * c) {
    return make(REPETITION, c);
}

// Parse a pattern and compare the augmented tree and symbol table
static void check_parse(const std::string &         pattern,
                        const std::string &         expected_tree,
                        const std::vector<u_char> & expected_symbols) {
    std::vector<u_char> symbol_at;
    Parser              p{ pattern, symbol_at };
    Node *              tree = p.parse();
    Node *              root = augment_pattern(tree, symbol_at);

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

static bool has_transition(const analyzed & a, const positions & from, u_char symbol, const positions & to) {
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

static size_t packed_transition(const DFA & dfa, size_t state, u_char symbol) {
    if (state >= dfa.base.size()) {
        return 0;
    }
    size_t index = dfa.base[state] + static_cast<u_char>(symbol);
    if (index >= dfa.check.size() || dfa.check[index] != state) {
        return 0;
    }
    return dfa.next_state[index];
}

static bool matches_entire(const std::string & pattern, const std::string & input) {
    DFA    dfa   = regex_to_dfa(pattern);
    size_t state = dfa.initstate;
    for (unsigned char byte : input) {
        state = packed_transition(dfa, state, byte);
        if (state == 0) {
            return false;
        }
    }
    return dfa.accepting.find(state) != dfa.accepting.end();
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
// Desugaring tests
// ---------------------------------------------------------------------------
static std::string to_hex(const std::string & s) {
    static const char digits[] = "0123456789abcdef";
    std::string       out;
    for (unsigned char c : s) {
        out += digits[c >> 4];
        out += digits[c & 0xF];
        out += ' ';
    }
    return out;
}

static void check_desugar(const std::string & input, const std::string & expected) {
    std::string pattern = input;
    desugar(pattern);
    if (pattern != expected) {
        printf("FAIL desugar '%s': got '%s', expected '%s'\n", input.c_str(), pattern.c_str(), expected.c_str());
        n_fail++;
    }
}

static void test_desugar_plus() {
    check_desugar("a+", "aa*");
}

static void test_desugar_group_plus() {
    check_desugar("(a|b)+", "(a|b)(a|b)*");
}

static void test_desugar_question() {
    check_desugar("a?", "(a|)");
}

static void test_desugar_question_group() {
    check_desugar("(a|b)?", "((a|b)|)");
}

static void test_desugar_exact_repetition() {
    check_desugar("a{3}", "aaa");
}

static void test_desugar_multi_digit_exact_repetition() {
    check_desugar("a{10}", "aaaaaaaaaa");
}

static void test_desugar_multi_digit_exact_repetition_group() {
    check_desugar("(a|b){10}", "(a|b)(a|b)(a|b)(a|b)(a|b)(a|b)(a|b)(a|b)(a|b)(a|b)");
}

static void test_desugar_open_ended_repetition() {
    check_desugar("a{2,}", "aaa*");
}

static void test_desugar_multi_digit_open_ended_repetition() {
    check_desugar("a{10,}", "aaaaaaaaaaa*");
}

static void test_desugar_bounded_repetition() {
    check_desugar("a{2,4}", "aa(a|)(a|)");
}

static void test_desugar_multi_digit_bounded_repetition() {
    check_desugar("a{10,12}", "aaaaaaaaaa(a|)(a|)");
}

static void test_desugar_bounded_repetition_group() {
    check_desugar("(a|b){2,4}", "(a|b)(a|b)((a|b)|)((a|b)|)");
}

static void test_desugar_multi_digit_bounded_repetition_group() {
    check_desugar("(a|b){10,12}", "(a|b)(a|b)(a|b)(a|b)(a|b)(a|b)(a|b)(a|b)(a|b)(a|b)((a|b)|)((a|b)|)");
}

static void test_desugar_bounded_repetition_edge() {
    check_desugar("a{0,2}", "(a|)(a|)");
}

static void test_desugar_identity_escape() {
    check_desugar("\\{", "{");
}

static void test_desugar_plain_symbol() {
    check_desugar("a", "a");
}

static void test_desugar_plain_pattern() {
    check_desugar("ab|c", "ab|c");
}

static void test_desugar_star_group() {
    check_desugar("(a|b)*", "(a|b)*");
}

static void test_desugar_character_class() {
    check_desugar("[abc]abc", "(a|b|c)abc");
}

static void test_desugar_range_class() {
    check_desugar("[a-d]abc", "(a|b|c|d)abc");
}

static void test_desugar_character_class_repetition() {
    check_desugar("[a-c]{4}", "(a|b|c)(a|b|c)(a|b|c)(a|b|c)");
}

static void test_desugar_unicode() {
    check_desugar("å*", "(å)*");
}

static void test_desugar_unicode_plus() {
    check_desugar("å+", "(å)(å)*");
}

static void test_desugar_unicode_question() {
    check_desugar("å?", "((å)|)");
}

static void test_desugar_unicode_class() {
    check_desugar("[é漢]", "(\xc3\xa9|\xe6\xbc\xa2)");
}

static void test_desugar_unicode_range_class() {
    check_desugar("[一-丄]", "(\xe4\xb8(\x80|\x81|\x82|\x83|\x84))");
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

static void test_parse_escaped_open_paren() {
    check_parse("\\(", "((.#)", { '(', '#' });
}

static void test_parse_escaped_close_paren() {
    check_parse("\\)", "().#)", { ')', '#' });
}

static void test_parse_group_with_escaped_open_paren() {
    check_parse("(\\()", "((.#)", { '(', '#' });
}

static void test_parse_group_with_escaped_close_paren() {
    check_parse("(\\))", "().#)", { ')', '#' });
}

static void test_parse_escaped_star() {
    check_parse("\\*", "(*.#)", { '*', '#' });
}

static void test_parse_escaped_alternation() {
    check_parse("\\|", "(|.#)", { '|', '#' });
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

    annotate(*root, followpos);

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

    annotate(*root, followpos);

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

    annotate(*root, followpos);

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

    annotate(*root, followpos);

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

static void test_unicode_character() {  // (æ|øå*)#
    analyzed          a      = analyze("æ|øå*");
    const std::string ae     = "æ";
    const std::string oe     = "ø";
    const std::string a_ring = "å";
    CHECK(a.dstates == std::set<positions>({ positions({ 0, 2 }), positions({ 1, 3 }), positions({ 4 }),
                                             positions({ 5, 6 }), positions({ 6 }) }));
    CHECK(has_transition(a, positions({ 0, 2 }), ae[0], positions({ 1, 3 })));
    CHECK(has_transition(a, positions({ 1, 3 }), ae[1], positions({ 6 })));
    CHECK(has_transition(a, positions({ 1, 3 }), oe[1], positions({ 4 })));
    CHECK(has_transition(a, positions({ 4 }), a_ring[0], positions({ 5, 6 })));
    CHECK(has_transition(a, positions({ 5, 6 }), a_ring[1], positions({ 5, 6 })));
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

    CHECK(dfa.alphabet == std::set<u_char>({ '#', 'a', 'b', 'c' }));
    CHECK(dfa.base.size() == 4);
    CHECK(packed_transition(dfa, 1, 'a') == 3);
    CHECK(packed_transition(dfa, 1, 'b') == 2);
    CHECK(packed_transition(dfa, 2, 'c') == 3);
    CHECK(packed_transition(dfa, 1, 'c') == 0);
    CHECK(packed_transition(dfa, 2, 'a') == 0);
}

static void test_regex_to_dfa_repetition() {
    DFA dfa = regex_to_dfa("a*");

    CHECK(dfa.alphabet == std::set<u_char>({ '#', 'a' }));
    CHECK(dfa.base.size() == 2);
    CHECK(packed_transition(dfa, 1, 'a') == 1);
    CHECK(packed_transition(dfa, 1, 'b') == 0);
}

static void test_regex_to_dfa_unicode_bytes() {
    const std::string ae  = "æ";
    DFA               dfa = regex_to_dfa(ae);

    size_t after_first_byte = packed_transition(dfa, dfa.initstate, ae[0]);
    CHECK(after_first_byte != 0);
    CHECK(packed_transition(dfa, after_first_byte, ae[1]) != 0);
}

static void test_unicode_codepoint_lengths() {
    const std::string two_byte   = "é";
    const std::string three_byte = "漢";
    const std::string four_byte  = "🙂";

    CHECK(two_byte.size() == 2);
    CHECK(three_byte.size() == 3);
    CHECK(four_byte.size() == 4);
    CHECK(matches_entire(two_byte, two_byte));
    CHECK(matches_entire(three_byte, three_byte));
    CHECK(matches_entire(four_byte, four_byte));
    CHECK(!matches_entire(two_byte, two_byte.substr(0, 1)));
    CHECK(!matches_entire(three_byte, three_byte.substr(0, 2)));
    CHECK(!matches_entire(four_byte, four_byte.substr(0, 3)));
}

static void test_unicode_concatenation_and_composition() {
    const std::string composed   = "café";
    const std::string decomposed = "cafe\xcc\x81";

    CHECK(matches_entire(composed, composed));
    CHECK(matches_entire(decomposed, decomposed));
    CHECK(!matches_entire(composed, decomposed));
}

static void test_unicode_quantifier_and_alternation() {
    const std::string acute = "é";
    const std::string han   = "漢";

    CHECK(matches_entire("é+", acute));
    CHECK(matches_entire("é+", acute + acute + acute));
    CHECK(!matches_entire("é+", ""));
    CHECK(matches_entire("é|漢", acute));
    CHECK(matches_entire("é|漢", han));
    CHECK(!matches_entire("é|漢", acute + han));
}

static void test_unicode_character_class() {
    const std::string acute = "é";
    const std::string han   = "漢";

    CHECK(matches_entire("[é漢]", acute));
    CHECK(matches_entire("[é漢]", han));
    CHECK(!matches_entire("[é漢]", "é漢"));
}

int main() {
    run("desugar_plus", test_desugar_plus);
    run("desugar_group_plus", test_desugar_group_plus);
    run("desugar_question", test_desugar_question);
    run("desugar_question_group", test_desugar_question_group);
    run("desugar_exact_repetition", test_desugar_exact_repetition);
    run("desugar_multi_digit_exact_repetition", test_desugar_multi_digit_exact_repetition);
    run("desugar_multi_digit_exact_repetition_group", test_desugar_multi_digit_exact_repetition_group);
    run("desugar_open_ended_repetition", test_desugar_open_ended_repetition);
    run("desugar_multi_digit_open_ended_repetition", test_desugar_multi_digit_open_ended_repetition);
    run("desugar_bounded_repetition", test_desugar_bounded_repetition);
    run("desugar_multi_digit_bounded_repetition", test_desugar_multi_digit_bounded_repetition);
    run("desugar_bounded_repetition_group", test_desugar_bounded_repetition_group);
    run("desugar_multi_digit_bounded_repetition_group", test_desugar_multi_digit_bounded_repetition_group);
    //run("desugar_bounded_repetition_edge", test_desugar_bounded_repetition_edge); //will fix later
    run("desugar_identity_escape", test_desugar_identity_escape);
    run("desugar_plain_symbol", test_desugar_plain_symbol);
    run("desugar_plain_pattern", test_desugar_plain_pattern);
    run("desugar_star_group", test_desugar_star_group);
    run("desugar_character_class", test_desugar_character_class);
    run("desugar_range_class", test_desugar_range_class);
    run("desugar_character_class_repetition", test_desugar_character_class_repetition);
    run("desugar_unicode", test_desugar_unicode);
    run("desugar_unicode_plus", test_desugar_unicode_plus);
    run("desugar_unicode_question", test_desugar_unicode_question);
    run("desugar_unicode_class", test_desugar_unicode_class);
    run("desugar_unicode_range_class", test_desugar_unicode_range_class);

    run("parse_empty", test_parse_empty);
    run("parse_symbol", test_parse_symbol);
    run("parse_escaped_open_paren", test_parse_escaped_open_paren);
    run("parse_escaped_close_paren", test_parse_escaped_close_paren);
    run("parse_group_with_escaped_open_paren", test_parse_group_with_escaped_open_paren);
    run("parse_group_with_escaped_close_paren", test_parse_group_with_escaped_close_paren);
    run("parse_escaped_star", test_parse_escaped_star);
    run("parse_escaped_alternation", test_parse_escaped_alternation);
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
    run("unicode_character", test_unicode_character);
    run("build_transition_table", test_build_transition_table);
    run("regex_to_dfa_alternation_concat", test_regex_to_dfa_alternation_concat);
    run("regex_to_dfa_repetition", test_regex_to_dfa_repetition);
    run("regex_to_dfa_unicode_bytes", test_regex_to_dfa_unicode_bytes);

    run("unicode_codepoint_lengths", test_unicode_codepoint_lengths);
    run("unicode_concatenation_and_composition", test_unicode_concatenation_and_composition);
    run("unicode_quantifier_and_alternation", test_unicode_quantifier_and_alternation);
    run("unicode_character_class", test_unicode_character_class);

    return n_fail == 0 ? 0 : 1;
}
