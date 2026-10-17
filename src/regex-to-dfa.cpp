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
Node * augment_pattern(Node * old_root, std::vector<char32_t> & symbol_at) {
    Node * special    = new Node;
    special->type     = REGEX_NODE_SYMBOL;
    special->value    = '#';
    special->position = symbol_at.size();

    Node * root       = new Node;
    root->type        = REGEX_NODE_CONCATENATION;
    root->left_child  = old_root;
    root->right_child = std::move(special);

    symbol_at.push_back('#');
    return root;
}

void desugar(std::string & pattern) {
    std::string pattern_cpy = pattern;
    pattern.clear();

    for (char32_t c : pattern_cpy) {
        switch (c)
        {
        case '+':
            pattern += pattern.back();
            pattern += '*';
            break;
        
        default:
            pattern += c;
        }
    }
    
    pattern = pattern;
    return;
}

Parser::Parser(const std::string & pattern, std::vector<char32_t> & symbol_at) : s(pattern), symbol_at(symbol_at) {}

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
    node->position = symbol_at.size();  // next free position
    symbol_at.push_back(c);
    return node;
}

void regex_annotate(Node & node, std::vector<std::set<size_t>> & followpos) {
    switch (node.type) {
        case REGEX_NODE_EPSILON:
            {
                // The empty string is of course empty
                node.nullable = true;
                break;
            }
        case REGEX_NODE_SYMBOL:
            {
                // A symbol is of course not empty
                node.nullable = false;

                // The first possible positions in a single symbol regex is just the position of that symbol.
                // same for lastpos
                node.firstpos.insert(node.position);
                node.lastpos.insert(node.position);
                break;
            }
        case REGEX_NODE_CONCATENATION:
            {
                // Start at leaf nodes
                regex_annotate(*node.left_child, followpos);
                regex_annotate(*node.right_child, followpos);

                // A concatenated pattern can only be nullable if both the concatenated patterns can
                node.nullable = node.left_child->nullable & node.right_child->nullable;

                // The first possible positions in a concatenated pattern must be firstpos of the lefthand side unless the lefthand side can be nothing, then firstpos is the union
                node.firstpos = node.left_child->firstpos;
                if (node.left_child->nullable) {
                    node.firstpos.insert(node.right_child->firstpos.begin(), node.right_child->firstpos.end());
                }

                // Lastpos is the same but in reverse
                node.lastpos = node.right_child->lastpos;
                if (node.right_child->nullable) {
                    node.lastpos.insert(node.left_child->lastpos.begin(), node.left_child->lastpos.end());
                }

                // Positions that are the last element in the lefthand side can be directly followed
                //by positions that are the first element in the righthand side
                for (size_t pos : node.left_child->lastpos) {
                    followpos.at(pos).insert(node.right_child->firstpos.begin(), node.right_child->firstpos.end());
                }
                break;
            }
        case REGEX_NODE_ALTERNATION:
            {
                // Start at leaf nodes
                regex_annotate(*node.left_child, followpos);
                regex_annotate(*node.right_child, followpos);

                // An alternated pattern can only be nullable if one of the patterns can
                node.nullable = node.left_child->nullable | node.right_child->nullable;

                // Similar to concatenation, but here firstpos is always the union
                node.firstpos = node.left_child->firstpos;
                node.firstpos.insert(node.right_child->firstpos.begin(), node.right_child->firstpos.end());

                // Lastpos is the same but in reverse
                node.lastpos = node.left_child->lastpos;
                node.lastpos.insert(node.right_child->lastpos.begin(), node.right_child->lastpos.end());
                break;
            }
        case REGEX_NODE_REPETITION:
            {
                // Start at leaf nodes
                regex_annotate(*node.left_child, followpos);

                // A pattern can be repeated zero times
                node.nullable = true;

                // Repetition changes nothing in firstpos or lastpos
                node.firstpos = node.left_child->firstpos;
                node.lastpos  = node.left_child->lastpos;

                // A repetition can either go back to the first position or move on.
                // Moving on is handled in other cases, so we just need to add firstpos
                for (size_t pos : node.left_child->lastpos) {
                    followpos.at(pos).insert(node.left_child->firstpos.begin(), node.left_child->firstpos.end());
                }
                break;
            }
    }
}

void followpos(std::string                  pattern,
               std::set<char32_t> &         alphabet,
               std::set<size_t> &           initial_state_position,
               std::set<std::set<size_t>> & accepting_state_positions,
               std::set<std::set<size_t>> & dstates,
               Transitions &                dtran) {
    // Parse the pattern, and return a syntax tree of the augmented pattern (r)# aswell as a lookup table `symbol_at`
    std::vector<char32_t> symbol_at;
    symbol_at.reserve(pattern.size());
    Parser p{ pattern, symbol_at };
    Node * tree = p.parse();
    Node * root = augment_pattern(tree, symbol_at);

    // Add terminal symbols found in the regex to the caller's alphabet
    alphabet.insert(symbol_at.begin(), symbol_at.end());

    // Calculate the functions `nullable`, `firstpos`, `lastpos` and `followpos`
    std::vector<std::set<size_t>> followpos(symbol_at.size());
    regex_annotate(*root, followpos);

    // Save initial state
    initial_state_position = root->firstpos;

    // Initialize with Dstates only containing firstpos(root)
    dstates.insert(root->firstpos);
    std::queue<std::set<size_t>> q;
    q.push(root->firstpos);

    free_tree(root);  //syntax tree destructor

    // Run algorithm
    std::set<size_t> state;
    while (!q.empty()) {
        state = q.front();  //pop from ToDo list
        q.pop();
        dtran[state] = {};  //initialize transition function for current state

        if (state.find(symbol_at.size() - 1) != state.end()) {
            accepting_state_positions.insert(state);
        }

        for (char32_t symbol : alphabet) {
            std::set<size_t> next_state;

            // Create a new state containing the positions that can be reached from the current state with the current symbol
            for (size_t position : state) {
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

            dtran.at(state).push_back({ next_state, symbol });
        }
    }
}

void build_transition_table(std::set<std::set<size_t>>         dstates,
                            Transitions                        dtran,
                            DFA &                              dfa,
                            const std::set<size_t> &           initial_state_position,
                            const std::set<std::set<size_t>> & accepting_state_positions) {
    // Sort states based on number of transitions comming from a state.
    //it makes packing more optimal to start with big states
    std::vector<std::set<size_t>> states(dstates.begin(), dstates.end());
    std::sort(states.begin(), states.end(), [&](const std::set<size_t> & a, const std::set<size_t> & b) {
        return dtran.at(a).size() > dtran.at(b).size();
    });

    // Now we can use vector indeces as state IDs instead of the full set
    // Create map from state "name" to ID
    std::map<std::set<size_t>, size_t> id_of;
    for (size_t i = 0; i < states.size(); ++i) {
        id_of[states[i]] = i + 1;  // id 0 is reserved for the dead state
    }

    // Reserve memory
    dfa.base.push_back(0);  //state id 0 is reserved for the dead state
    dfa.check.resize(
        states.size() +
        150);  //room for all states, plus full ASCII alphabet, plus some wiggleroom. Will probably need to resize in the following for loop
    dfa.next_state.resize(states.size() + 150);
    dfa.base.reserve(states.size() + 1);

    // Pack states
    size_t array_size = dfa.next_state.size();
    size_t state_id;
    bool   index_collision;
    for (size_t i = 0; i < states.size(); i++) {
        state_id = i + 1;

        // Check if there is room to pack state
        size_t offset = 0;
        do {
            index_collision = false;
            for (const auto & tran : dtran[states[i]]) {
                char32_t a = tran.symbol;

                while (offset + a >= array_size) {
                    array_size *= 2;  //if out-of-index, double the size
                }
                dfa.check.resize(array_size);
                dfa.next_state.resize(array_size);

                if (dfa.check.at(offset + a) != 0) {
                    index_collision = true;
                    offset++;
                    break;
                }
            }
        } while (index_collision);

        // Pack state
        for (const auto & tran : dtran[states[i]]) {
            char32_t a                    = tran.symbol;
            dfa.check.at(offset + a)      = state_id;
            dfa.next_state.at(offset + a) = id_of[tran.to];
        }

        // Save base for this state
        dfa.base.push_back(offset);
    }

    dfa.initstate = id_of.at(initial_state_position);
    for (const auto & accepting_state : accepting_state_positions) {
        dfa.accepting.insert(id_of.at(accepting_state));
    }
}

DFA regex_to_dfa(std::string pattern) {
    // Remove syntactic sugar
    desugar(pattern);

    // Execute followpos
    DFA                        dfa;
    std::set<std::set<size_t>> dstates = {};
    Transitions                dtran;
    std::set<size_t>           initial_state_position;
    std::set<std::set<size_t>> accepting_state_positions;
    followpos(pattern, dfa.alphabet, initial_state_position, accepting_state_positions, dstates, dtran);

    // Potentially find minimal DFA
    // ...
    // Probably not worth it for the llama.cpp common case

    // Minimize memory usage of transition table
    build_transition_table(dstates, dtran, dfa, initial_state_position, accepting_state_positions);

    return dfa;
}
