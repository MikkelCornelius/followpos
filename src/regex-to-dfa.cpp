#include "regex-to-dfa.h"

#include <stdexcept>

namespace regex_dfa {

static size_t find_group(std::string & pattern, size_t sub_pattern_start_index) {
    size_t count = 1;  // Start at 1 for the opening parenthesis we're matching

    for (size_t i = sub_pattern_start_index; i < pattern.length(); i++) {
        if (pattern[i] == '(') {
            count++;
        } else if (pattern[i] == ')') {
            count--;
            if (count == 0) {
                return i;  // Found the matching closing parenthese
            }
        }
    }
    throw std::logic_error("unclosed parenthese");
}

static size_t utf8_length(u_char c) {
    if (c < 0x80) {
        return 1;  // 0xxxxxxx
    }
    if ((c >> 5) == 0b110) {
        return 2;  // 110xxxxx
    }
    if ((c >> 4) == 0b1110) {
        return 3;  // 1110xxxx
    }
    if ((c >> 3) == 0b11110) {
        return 4;  // 11110xxx
    }
    return 1;      // continuation byte
}

void desugar(std::string & pattern) {
    std::string pattern_cpy = pattern;
    pattern.clear();  //make the pattern empty to later rebuild it

    char    c;
    std::string group = "";
    size_t      end;
    for (size_t i = 0; i < pattern_cpy.length(); i++) {
        // Concatenate bytes if multiple bytes are required to represent current character
        size_t char_size = utf8_length(pattern_cpy[i]);
        if (char_size > 1) {
            group.clear();
            pattern += '(';
            for (size_t j = 0; j < char_size; j++) {
                pattern += pattern_cpy[i];
                group += pattern_cpy[i];
                i++;
            }
            i--;
            pattern += ')';
            continue;
        }

        c = pattern_cpy[i];
        switch (c) {
            case '(':
                if (pattern_cpy[i + 1] == '?') {
                    throw std::logic_error("lookahead not yet supported");
                }
                end   = find_group(pattern_cpy, i + 1);
                group = pattern_cpy.substr(i + 1, end - i - 1);
                desugar(group);                //recursively dig into groups
                pattern += "(" + group + ")";  //append the desugared group
                i = end;
                continue;                      //save group for next iteration

            case '+':
                if (group.size() > 1) {  //add parenthesis if group is not a single character or single byte
                    pattern += "(" + group + ")*";
                } else {
                    pattern += group + "*";
                }
                break;

            case '?':
                if (group.size() > 1) {
                    // We need to insert a '()' before the group
                    pattern.erase(pattern.size() - group.size() - 2);
                    pattern += "((" + group + ")|)";
                } else {
                    c = pattern.back();
                    pattern.pop_back();
                    pattern += '(';
                    pattern += c;
                    pattern += "|)";
                }
                break;

            case '{':                                                    //quantifier
                i++;
                if (group.size() > 1) {                         //if group
                    for (size_t n = pattern_cpy[i] - '0'; n > 1; n--) {  //{n}
                        pattern += "(" + group + ")";
                    }
                    i++;
                    if (pattern_cpy[i] == '}') {  //{n}
                        if (pattern_cpy[i + 1] == '?') {
                            throw std::logic_error("non-greedy quantifier not yet supported");
                        }
                        break;
                    }
                    i++;
                    if (pattern_cpy[i] == '}') {  //{n,}
                        pattern += "(" + group + ")*";
                        if (pattern_cpy[i + 1] == '?') {
                            throw std::logic_error("non-greedy quantifier not yet supported");
                        }
                        break;
                    } else {  //{n,m}
                        for (int m = pattern_cpy[i] - '0'; m > pattern_cpy[i - 2] - '0'; m--) {
                            pattern += "((" + group + ")|)";
                        }
                    }
                } else {                                                 //if not a group
                    for (size_t n = pattern_cpy[i] - '0'; n > 1; n--) {  //{n}
                        pattern += group;
                    }
                    i++;
                    if (pattern_cpy[i] == '}') {  //{n}
                        if (pattern_cpy[i + 1] == '?') {
                            throw std::logic_error("non-greedy quantifier not yet supported");
                        }
                        break;
                    }
                    i++;
                    if (pattern_cpy[i] == '}') {  //{n,}
                        pattern += group + "*";
                        if (pattern_cpy[i + 1] == '?') {
                            throw std::logic_error("non-greedy quantifier not yet supported");
                        }
                        break;
                    } else {  //{n,m}
                        for (int m = pattern_cpy[i] - '0'; m > pattern_cpy[i - 2] - '0'; m--) {
                            pattern += "(" + group + "|)";
                        }
                    }
                }
                i++;
                if (pattern_cpy[i + 1] == '?') {
                    throw std::logic_error("non-greedy quantifier not yet supported");
                }
                break;

            case '.':
                throw std::logic_error("classes not yet supported");
                break;

            case '[':
                i++;
                if (pattern_cpy[i] == '^') {
                    throw std::logic_error("negated classes not yet supported");
                }
                if (pattern_cpy[i] == ':') {
                    throw std::logic_error("POSIX-based character classes not yet supported");
                }

                // "[abc]" becomes "(a|b|c)"
                pattern += '(';
                while (pattern_cpy[i] != ']') {
                    if (pattern_cpy[i] == '-') {
                        throw std::logic_error("ranges '[a-z]' not yet supported");
                    }

                    // "[æøå]" becomes "(\xc3\xa6|\xc3\xb8|\xc3\xa5)"
                    char_size = utf8_length(pattern_cpy[i]);
                    for (size_t j = 0; j < char_size; j++) {
                        pattern += pattern_cpy[i];
                        i++;
                    }
                    pattern += '|';
                }
                pattern.pop_back();  //remove trailing '|'
                pattern += ')';
                break;

            case '^':
                throw std::logic_error("start of input '^' not yet supported");
                break;

            case '$':
                throw std::logic_error("end of input '$' not yet supported");
                break;

            case '\\':
                throw std::logic_error("escape '\\' not yet supported");
                i++;
                switch (pattern_cpy[i]) {
                    case 'd':
                        throw std::logic_error("digit class '\\d' not yet supported");
                        break;

                    case 'D':
                        throw std::logic_error("digit class '\\D' not yet supported");
                        break;

                    case 's':
                        throw std::logic_error("whitespace class '\\s' not yet supported");
                        break;

                    case 'S':
                        throw std::logic_error("whitespace class '\\S' not yet supported");
                        break;

                    case 'w':
                        throw std::logic_error("word class '\\w' not yet supported");
                        break;

                    case 'W':
                        throw std::logic_error("word class '\\W' not yet supported");
                        break;

                    case 'b':
                        throw std::logic_error("word boundary '\\b' not yet supported");
                        break;

                    case 'B':
                        throw std::logic_error("word boundary '\\B' not yet supported");
                        break;

                    case 'f':  //form feed
                        pattern += '\f';
                        break;

                    case 'n':  //newline
                        pattern += '\n';
                        break;

                    case 'r':  //carriage return
                        pattern += '\r';
                        break;

                    case 't':  //horizontal tab
                        pattern += '\t';
                        break;

                    case 'v':  //vertical tab
                        pattern += '\v';
                        break;

                    case '0':  //NUL
                        pattern += '\0';
                        break;

                    case 'c':  //character (\cX)
                        i++;
                        pattern += pattern_cpy[i];
                        break;

                    case 'x':  //character (\xHH)
                        pattern += static_cast<char>(std::stoul(pattern_cpy.substr(i + 1, 2), nullptr, 16));
                        i += 2;
                        break;

                    case 'u':                                                                  //character (\uHHHH)
                        throw std::logic_error("control characters '\\u' not yet supported");  //implementation buggy
                        pattern += static_cast<char32_t>(std::stoul(pattern_cpy.substr(i + 1, 4), nullptr, 16));
                        i += 4;
                        break;

                    case '1':
                    case '2':
                    case '3':
                    case '4':
                    case '5':
                    case '6':
                    case '7':
                    case '8':
                    case '9':
                        throw std::logic_error("backreferences not yet supported");
                        break;

                    default:  //IdentityEscape
                        pattern += c;
                }
                break;

            default:  //no suntactic sugar, just append original character
                pattern += c;
        }
        group = pattern.back();  //if no parenthese, group will just be one character
    }
    return;
}

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
Node * augment_pattern(Node * old_root, std::vector<u_char> & symbol_at) {
    Node * special    = new Node;
    special->type     = SYMBOL;
    special->value    = '#';
    special->position = symbol_at.size();

    Node * root       = new Node;
    root->type        = CONCATENATION;
    root->left_child  = old_root;
    root->right_child = std::move(special);

    symbol_at.push_back('#');
    return root;
}

Parser::Parser(const std::string & pattern, std::vector<u_char> & symbol_at) : s(pattern), symbol_at(symbol_at) {}

u_char Parser::peek() const {
    return i < s.size() ? s[i] : '\0';
}

Node * Parser::make(node_type type, Node * left, Node * right) {
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
        left = make(ALTERNATION, left, parse_cat());
    }
    return left;
}

Node * Parser::parse_cat() {
    Node * left = nullptr;
    while (peek() != '\0' && peek() != '|' && peek() != ')') {
        Node * right = parse_postfix();
        left         = left ? make(CONCATENATION, left, right) : right;
    }
    return left ? left : make(EPSILON, nullptr);
}

Node * Parser::parse_postfix() {
    Node * node = parse_atom();
    while (peek() == '*') {
        i++;
        node = make(REPETITION, node);
    }
    return node;
}

Node * Parser::parse_atom() {
    if (i >= s.size()) {
        throw std::logic_error("unexpected end of pattern");
    }

    u_char c = s[i++];
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

    Node * node    = make(SYMBOL, nullptr);
    node->value    = c;
    node->position = symbol_at.size();  // next free position
    symbol_at.push_back(c);
    return node;
}

void regex_annotate(Node & node, std::vector<std::set<size_t>> & followpos) {
    switch (node.type) {
        case EPSILON:
            {
                // The empty string is of course empty
                node.nullable = true;
                break;
            }
        case SYMBOL:
            {
                // A symbol is of course not empty
                node.nullable = false;

                // The first possible positions in a single symbol regex is just the position of that symbol.
                // same for lastpos
                node.firstpos.insert(node.position);
                node.lastpos.insert(node.position);
                break;
            }
        case CONCATENATION:
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
        case ALTERNATION:
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
        case REPETITION:
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
               std::set<u_char> &             alphabet,
               std::set<size_t> &           initial_state_position,
               std::set<std::set<size_t>> & accepting_state_positions,
               std::set<std::set<size_t>> & dstates,
               Transitions &                dtran) {
    // Parse the pattern, and return a syntax tree of the augmented pattern (r)# aswell as a lookup table `symbol_at`
    std::vector<u_char> symbol_at;
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

    // NFA->DFA subset construction
    std::set<size_t> state;
    while (!q.empty()) {
        state = q.front();  //pop from ToDo list
        q.pop();
        dtran[state] = {};  //initialize transition function for current state

        if (state.find(symbol_at.size() - 1) != state.end()) {
            accepting_state_positions.insert(state);
        }

        for (u_char symbol : alphabet) {
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
                size_t symbol = static_cast<u_char>(tran.symbol);

                while (offset + symbol >= array_size) {
                    array_size *= 2;  //if out-of-index, double the size
                }
                dfa.check.resize(array_size);
                dfa.next_state.resize(array_size);

                if (dfa.check.at(offset + symbol) != 0) {
                    index_collision = true;
                    offset++;
                    break;
                }
            }
        } while (index_collision);

        // Pack state
        for (const auto & tran : dtran[states[i]]) {
            size_t symbol                      = static_cast<u_char>(tran.symbol);
            dfa.check.at(offset + symbol)      = state_id;
            dfa.next_state.at(offset + symbol) = id_of[tran.to];
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

//
// Runtime
//

static bool advance_dfa(DFA & dfa, size_t & current_state, u_char character) {
    // TODO there might be a bug if `character` is outside the alphabet
    if (current_state >= dfa.base.size()) {
        return false;
    }

    size_t index = dfa.base[current_state] + character;
    if (index >= dfa.check.size()) {
        return 0;
    }
    if (dfa.check[index] == current_state) {
        current_state = dfa.next_state[index];
        return true;
    }
    return false;  //advance failed (dead state)
}

static bool advance_dfas(DFA & dfa, std::list<size_t> & live_dfas, u_char character) {
    live_dfas.push_back(dfa.initstate);
    for (auto it = live_dfas.begin(); it != live_dfas.end();) {
        size_t & current_state = *it;
        if (!advance_dfa(dfa, current_state, character)) {  //if advance fails kill this DFA
            it = live_dfas.erase(it);
        } else {
            ++it;
        }
    }

    for (size_t state : live_dfas) {
        if (dfa.accepting.find(state) != dfa.accepting.end()) {
            return true;
        }
    }
    return false;
}

bool feed(const std::string & tok, DFA & dfa, std::list<size_t> & live_dfas) {
    for (u_char character : tok) {
        if (advance_dfas(dfa, live_dfas, character)) {
            return true;
        }
    }
    return false;
}

}  // namespace regex_dfa
