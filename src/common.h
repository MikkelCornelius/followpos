#pragma once

#include <algorithm>  //for a sorting algorithm. A non-sorted vector would also be okay
#include <map>
#include <queue>
#include <set>
#include <string>
#include <vector>

enum regex_node_type {
    REGEX_NODE_EPSILON       = 0,
    REGEX_NODE_SYMBOL        = 1,
    REGEX_NODE_CONCATENATION = 2,
    REGEX_NODE_ALTERNATION   = 3,
    REGEX_NODE_REPETITION    = 4
};

struct Node {
    regex_node_type  type        = REGEX_NODE_EPSILON;
    char             value       = '\0';
    int              position    = -1;
    Node *           left_child  = nullptr;
    Node *           right_child = nullptr;
    bool             nullable;
    std::set<size_t> firstpos;
    std::set<size_t> lastpos;
};

void free_tree(Node * node);

struct Parser {
    Parser(const std::string & pattern, std::vector<char> & symbol_at);

    Node * parse();

    const std::string & s;
    std::vector<char> & symbol_at;
    size_t              i = 0;

    char   peek() const;
    Node * make(regex_node_type type, Node * left, Node * right = nullptr);

    Node * parse_or();
    Node * parse_cat();
    Node * parse_postfix();
    Node * parse_atom();
};

struct Transition {
    std::set<size_t> to;
    char             symbol;
};

struct DFA {
    std::set<char>      alphabet;
    std::vector<size_t> next_state;
    std::vector<size_t> check;
    std::vector<size_t> base;
    size_t              initstate;
    std::set<size_t>    accepting = {};
};

using Transitions = std::map<std::set<size_t>, std::vector<Transition>>;

Node * augment_pattern(Node * old_root, std::vector<char> & symbol_at);

void desugar(std::string & pattern);

void regex_annotate(Node & node, std::vector<std::set<size_t>> & followpos);

void followpos(std::string                  pattern,
               std::set<char> &             alphabet,
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
