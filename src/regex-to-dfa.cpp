#include "common.h"

#include <stdexcept>

static void desugar(std::string & pattern) {
    throw std::logic_error("feature not implemented");
}

Node regex_parse(std::string & pattern, std::vector<char32_t> * symbol_at) {
    desugar(pattern);
    throw std::logic_error("feature not implemented");
}

void regex_annotate(Node * node, std::vector<std::set<int>> * followpos) {
    switch (node->type) {
        case REGEX_NODE_EPSILON:
            {
                throw std::logic_error("feature not implemented");
                break;
            }
        case REGEX_NODE_SYMBOL:
            {
                throw std::logic_error("feature not implemented");
                break;
            }
        case REGEX_NODE_CONCATENATION:
            {
                regex_annotate(&node->children[0], followpos);
                regex_annotate(&node->children[1], followpos);
                throw std::logic_error("feature not implemented");
                break;
            }
        case REGEX_NODE_ALTERNATION:
            {
                regex_annotate(&node->children[0], followpos);
                regex_annotate(&node->children[1], followpos);
                throw std::logic_error("feature not implemented");
                break;
            }
        case REGEX_NODE_REPETITION:
            {
                regex_annotate(&node->children[0], followpos);
                throw std::logic_error("feature not implemented");
                break;
            }
    }
}

DFA regex_to_dfa(std::string pattern) {
    // Parse the pattern, and return a syntax tree of the augmented pattern (r)# aswell as a lookup table `symbol_at`
    std::vector<char32_t> symbol_at;
    Node root = regex_parse(pattern, &symbol_at);

    // Define an alphabet to only contain terminal symbols found in the regex
    std::set<char32_t> alphabet(symbol_at.begin(), symbol_at.end());

    // Calculate the functions `nullable`, `firstpos`, `lastpos` and `followpos`
    std::vector<std::set<int>> followpos(symbol_at.size());
    regex_annotate(&root, &followpos);

    // Initialize with Dstates only containing firstpos(root)
    std::set<std::set<int>>   dstates = { root.firstpos };
    Transitions               dtran;
    std::queue<std::set<int>> q;
    q.push(root.firstpos);

    // Run algorithm
    std::set<int> state;
    while (!q.empty()) {
        state = q.front();  //pop from ToDo list
        q.pop();
        dtran[state] = {};  //initialize transition function for current state

        for (char32_t symbol : alphabet) {
            std::set<int> next_state;

            // Create a new state containing the positions that can be reached from the current state with the current symbol
            for (int position : state) {
                if (symbol_at[position] == symbol) {
                    next_state.insert(followpos[position].begin(), followpos[position].end());
                }
            }

            if (next_state.empty()) {
                continue;
            }

            // If `next_state` not already in `Dstates`, add it and add to the ToDo list
            if (dstates.find(next_state) == dstates.end()) {
                dstates.insert(next_state);
                q.push(next_state);
            }

            dtran[state].push_back({ next_state, symbol });
        }
    }

    return { 0 };
}