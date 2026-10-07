#include "common.h"

#include <stdexcept>

// Clean up memory from syntax tree
void free_tree(Node * node) {
    if (node == nullptr) {
        return;
    }
    free_tree(node->left_child);
    free_tree(node->right_child);
    delete node;
}

// The augmented pattern "(r)#"
Node * augment_pattern(Node * old_root, std::vector<char32_t> * symbol_at) {
    Node * special    = new Node;
    special->type     = REGEX_NODE_SYMBOL;
    special->value    = '#';
    special->position = symbol_at->size();

    Node * root       = new Node;
    root->type        = REGEX_NODE_CONCATENATION;
    root->left_child  = std::move(old_root);
    root->right_child = std::move(special);

    symbol_at->push_back('#');
    return root;
}

void desugar(std::string & pattern) {
    throw std::logic_error("feature not implemented");
}

static int find_group(std::string pattern, int sub_pattern_start_index) {
    int count = 1;  // Start at 1 for the opening parenthesis we're matching

    for (int i = sub_pattern_start_index; i < pattern.length(); i++) {
        if (pattern[i] == '(') {
            count++;
        } else if (pattern[i] == ')') {
            count--;
            if (count == 0) {
                return i;  // Found the matching closing paren
            }
        }
    }
    return -1;  // No matching closing paren found
}

Node * regex_parse(std::string & pattern, std::vector<char32_t> * symbol_at) {
    //desugar(pattern);

    Node * current_node = nullptr;
    Node * new_branch;
    Node * next_node = nullptr;
    int    i         = 1;

    if (pattern.empty()) {
        next_node       = new Node;
        next_node->type = REGEX_NODE_EPSILON;
        return next_node;
    }

    // Start by initalizing the tree. Three cases: group "(x)", special "\", or symbol
    bool initalized = false;
    while (pattern[i - 1] == '(') {  //can't initalize a group. Instead dig in and initalize a subpattern
        initalized = true;           //if while loop ran at least once

        // Find sub pattern
        int         end         = find_group(pattern.c_str(), i);
        std::string sub_pattern = pattern.substr(i, end - i);

        if (next_node ==
            nullptr) {  //if first iteration, call `regex_parse()` and use generated syntax tree as the initializer
            // Parse sub pattern
            next_node = regex_parse(sub_pattern, symbol_at);

        } else {  //else concatenate subtree
            current_node           = next_node;
            next_node              = new Node;
            next_node->type        = REGEX_NODE_CONCATENATION;
            next_node->left_child  = current_node;
            next_node->right_child = regex_parse(sub_pattern, symbol_at);
        }

        // Continue from next operator
        i = end + 2;
    }
    if (!initalized) {
        if (pattern[0] == '\\') {
            throw std::logic_error("feature not implemented");

        } else {  //symbol
            next_node           = new Node;
            next_node->type     = REGEX_NODE_SYMBOL;
            next_node->value    = pattern[0];
            next_node->position = 0;
            symbol_at->push_back(pattern[0]);
        }
    }

    int pos = 0;
    for (i; i < pattern.length(); i++) {  //start at index 1 insted of 0 to get operator
        current_node = next_node;
        switch (pattern[i]) {
            case '|':
                i++;  //increment one more time to get symbol
                new_branch           = new Node;
                new_branch->type     = REGEX_NODE_SYMBOL;
                new_branch->value    = pattern[i];
                new_branch->position = pos++;
                symbol_at->push_back(pattern[i]);

                next_node              = new Node;
                next_node->type        = REGEX_NODE_ALTERNATION;
                next_node->left_child  = current_node;
                next_node->right_child = new_branch;
                break;

            case '*':
                next_node             = new Node;
                next_node->type       = REGEX_NODE_REPETITION;
                next_node->left_child = current_node;
                break;

            case '(':
                throw std::logic_error("feature not implemented");
                break;

            default:  //symbol
                new_branch           = new Node;
                new_branch->type     = REGEX_NODE_SYMBOL;
                new_branch->value    = pattern[i];
                new_branch->position = pos++;
                symbol_at->push_back(pattern[i]);

                next_node              = new Node;
                next_node->type        = REGEX_NODE_CONCATENATION;
                next_node->left_child  = current_node;
                next_node->right_child = new_branch;
                break;
        }
    }

    return next_node;
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
                regex_annotate(node->left_child, followpos);
                regex_annotate(node->right_child, followpos);
                throw std::logic_error("feature not implemented");
                break;
            }
        case REGEX_NODE_ALTERNATION:
            {
                regex_annotate(node->left_child, followpos);
                regex_annotate(node->right_child, followpos);
                throw std::logic_error("feature not implemented");
                break;
            }
        case REGEX_NODE_REPETITION:
            {
                regex_annotate(node->left_child, followpos);
                throw std::logic_error("feature not implemented");
                break;
            }
    }
}

DFA regex_to_dfa(std::string pattern) {
    // Parse the pattern, and return a syntax tree of the augmented pattern (r)# aswell as a lookup table `symbol_at`
    std::vector<char32_t> symbol_at;
    symbol_at.reserve(pattern.size());
    Node * tree = regex_parse(pattern, &symbol_at);
    Node * root = augment_pattern(tree, &symbol_at);

    // Define an alphabet to only contain terminal symbols found in the regex
    std::set<char32_t> alphabet(symbol_at.begin(), symbol_at.end());

    // Calculate the functions `nullable`, `firstpos`, `lastpos` and `followpos`
    std::vector<std::set<int>> followpos(symbol_at.size());
    regex_annotate(root, &followpos);

    // Initialize with Dstates only containing firstpos(root)
    std::set<std::set<int>>   dstates = { root->firstpos };
    Transitions               dtran;
    std::queue<std::set<int>> q;
    q.push(root->firstpos);

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

    free_tree(root);  //syntax tree desctructor

    return { 0 };
}
