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
    char32_t         value       = '\0';
    int              position    = -1;
    Node *           left_child  = nullptr;
    Node *           right_child = nullptr;
    bool             nullable;
    std::set<size_t> firstpos;
    std::set<size_t> lastpos;
};

void free_tree(Node * node);

struct Parser {
    Parser(const std::string & pattern, std::vector<char32_t> & symbol_at);

    Node * parse();

    const std::string &     s;
    std::vector<char32_t> & symbol_at;
    size_t                  i = 0;

    char   peek() const;
    Node * make(regex_node_type type, Node * left, Node * right = nullptr);

    Node * parse_or();
    Node * parse_cat();
    Node * parse_postfix();
    Node * parse_atom();
};

struct Transition {
    std::set<size_t> to;
    char32_t         symbol;
};

struct DFA {
    std::vector<size_t> next_state;
    std::vector<size_t> check;
    std::vector<size_t> base;
};

using Transitions = std::map<std::set<size_t>, std::vector<Transition>>;

Node * augment_pattern(Node * old_root, std::vector<char32_t> & symbol_at);

void regex_annotate(Node & node, std::vector<std::set<size_t>> & followpos);

void followpos(std::string pattern,
               std::set<char32_t> & alphabet,
               std::set<std::set<size_t>> & dstates,
               Transitions & dtran);

void build_transition_table(std::set<std::set<size_t>> dstates,
                            Transitions                dtran,
                            std::vector<size_t> &      next_state,
                            std::vector<size_t> &      check,
                            std::vector<size_t> &      base);

DFA regex_to_dfa(std::string pattern);
