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

Parser::Parser(const std::string & pattern, std::vector<char32_t> * symbol_at) : s(pattern), symbol_at(symbol_at) {}

char Parser::peek() const {
    return i < s.size() ? s[i] : '\0';
}

Node * Parser::make(regex_node_type type, Node * left, Node * right) {
    Node * n       = new Node;
    n->type        = type;
    n->left_child  = left;
    n->right_child = right;
    return n;
}

Node * Parser::parse() {
    Node * tree = parse_or();
    if (i < s.size()) {
        throw std::logic_error("unexpected character in pattern");  // e.g. a stray ')'
    }
    return tree;
}

Node * Parser::parse_or() {
    Node * left = parse_cat();
    while (peek() == '|') {
        i++;
        left = make(REGEX_NODE_ALTERNATION, left, parse_cat());
    }
    return left;
}

Node * Parser::parse_cat() {
    Node * left = nullptr;
    while (peek() != '\0' && peek() != '|' && peek() != ')') {
        Node * right = parse_postfix();
        left         = left ? make(REGEX_NODE_CONCATENATION, left, right) : right;
    }
    return left ? left : make(REGEX_NODE_EPSILON, nullptr);
}

Node * Parser::parse_postfix() {
    Node * node = parse_atom();
    while (peek() == '*') {
        i++;
        node = make(REGEX_NODE_REPETITION, node);
    }
    return node;
}

Node * Parser::parse_atom() {
    if (i >= s.size()) {
        throw std::logic_error("unexpected end of pattern");
    }

    char c = s[i++];
    if (c == '(') {
        Node * node = parse_or();
        if (peek() != ')') {
            throw std::logic_error("missing )");
        }
        i++;
        return node;
    }
    if (c == '*') {
        throw std::logic_error("nothing to repeat");
    }
    if (c == '\\') {
        throw std::logic_error("feature not implemented");
    }

    Node * node    = make(REGEX_NODE_SYMBOL, nullptr);
    node->value    = c;
    node->position = symbol_at->size();  // next free position
    symbol_at->push_back(c);
    return node;
}



Node * regex_parse(std::string & pattern, std::vector<char32_t> * symbol_at) {
    Parser p{ pattern, symbol_at };
    return p.parse();
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

    // Remove syntactic sugar
    //desugar(pattern);

    // Parse the pattern, and return a syntax tree of the augmented pattern (r)# aswell as a lookup table `symbol_at`
    std::vector<char32_t> symbol_at;
    symbol_at.reserve(pattern.size());
    Parser p{ pattern, &symbol_at };
    Node * tree = p.parse();
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
