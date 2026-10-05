#pragma once

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
    regex_node_type type;
    char32_t        value;
    int             position;
    std::vector<Node> children;
    bool            nullable;
    std::set<int>   firstpos;
    std::set<int>   lastpos;
};

struct Transition {
    std::set<int> to;
    char32_t      symbol;
};

struct DFA {
    int dummy;  //not yet implemented
};

using Transitions = std::map<std::set<int>, std::vector<Transition>>;

Node regex_parse(std::string & pattern, std::vector<char32_t> * symbol_at);

void regex_annotate(Node * node, std::vector<std::set<int>> * followpos);

DFA regex_to_dfa(std::string pattern);

bool regex_dfa_match(const DFA & dfa, const std::string & text);