#pragma once

#include <algorithm>  //for a sorting algorithm. A non-sorted vector would also be okay
#include <list>
#include <map>
#include <queue>
#include <set>
#include <string>
#include <vector>

namespace regex_dfa {

enum node_type { EPSILON = 0, SYMBOL = 1, CONCATENATION = 2, ALTERNATION = 3, REPETITION = 4 };

struct Node {
    node_type        type        = EPSILON;
    u_char           value       = '\0';
    int              position    = -1;
    Node *           left_child  = nullptr;
    Node *           right_child = nullptr;
    bool             nullable;
    std::set<size_t> firstpos;
    std::set<size_t> lastpos;
};

void free_tree(Node * node);

struct Parser {
    Parser(const std::string & pattern, std::vector<u_char> & symbol_at);

    Node * parse();

    const std::string &   s;
    std::vector<u_char> & symbol_at;
    size_t                i = 0;

    u_char peek() const;
    Node * make(node_type type, Node * left, Node * right = nullptr);

    Node * parse_or();
    Node * parse_cat();
    Node * parse_postfix();
    Node * parse_atom();
};

struct Transition {
    std::set<size_t> to;
    u_char           symbol;
};

struct DFA {
    std::set<u_char>    alphabet;
    std::vector<size_t> next_state;
    std::vector<size_t> check;
    std::vector<size_t> base;
    size_t              initstate;
    std::set<size_t>    accepting = {};
};

using Transitions = std::map<std::set<size_t>, std::vector<Transition>>;

struct ByteRange {
    size_t lo, hi;
};

using Utf8Seq = std::vector<ByteRange>;

Node * augment_pattern(Node * old_root, std::vector<u_char> & symbol_at);

void desugar(std::string & pattern);

void annotate(Node & node, std::vector<std::set<size_t>> & followpos);

void followpos(std::string                  pattern,
               std::set<u_char> &           alphabet,
               std::set<size_t> &           initial_state_positions,
               std::set<std::set<size_t>> & accepting_state_positions,
               std::set<std::set<size_t>> & dstates,
               Transitions &                dtran);

void build_transition_table(std::set<std::set<size_t>>         dstates,
                            Transitions                        dtran,
                            DFA &                              dfa,
                            const std::set<size_t> &           initial_state_positions,
                            const std::set<std::set<size_t>> & accepting_state_positions);

DFA regex_to_dfa(std::string pattern);

bool feed(const std::string & tok, DFA & dfa, std::list<size_t> & live_dfas);
}  // namespace regex_dfa
