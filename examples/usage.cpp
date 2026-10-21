#include "regex-to-dfa.h"
#include <iostream>

#include <fstream>
#include <iomanip>

std::vector<std::string> read_tokens(const std::string& path) {
    std::vector<std::string> tokens;
    std::ifstream file(path);
    std::string tok;
    char comma;
    while (file >> std::quoted(tok)) {
        tokens.push_back(tok);
        file >> comma;
    }
    return tokens;
}

int main() {
    std::vector<std::string> content = read_tokens("examples/tokens_word_pat.txt");
    std::string pat = "<tool_call>";

    regex_dfa::DFA    dfa = regex_dfa::regex_to_dfa(pat);
    std::list<size_t> live_dfas;

    for (std::string tok : content) {
        if (regex_dfa::feed(tok, dfa, live_dfas)) {
            std::cout << "match found at token: " << tok << std::endl;
            break;
        }
    }
}
