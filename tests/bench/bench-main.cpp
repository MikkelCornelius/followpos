#include "bench.h"

#include <fstream>
#include <iomanip>
#include <iostream>

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

static void report(const char* name, const BenchResult& r) {
    std::cout << name << ": init " << r.init_us << "us, "
              << r.match_ns / (long) r.tokens << "ns/token"
              << std::endl;
}

int main() {
    std::vector<std::string> files;
    std::vector<std::string> patterns;
    
    // Natural language
    files.insert(files.end(), {"english", "danish", "german", "spanish", "greek", "japanese", "english_regex"});
    patterns.insert(patterns.end(), {"<tool_call>", "<funktionskald>", "<funktionsaufruf>", "<llamada_herramienta>", "<κλήση_εργαλείου>", "<ツール呼び出し>", "<tool_call>\\s*\\{\\s*\"name\""});

    // Formal language
    files.insert(files.end(), {"python", "https"});
    patterns.insert(patterns.end(), {"(eval|exec)\\s*\\(", "(%3[Cc]|<)[sS][cC][rR][iI][pP][tT](%3[Ee]|>)"});

    // Custom language
    files.insert(files.end(), {"long", "backtrack", "state_explosion", "alphabet_test", "alphabet_test_japanese"});
    patterns.insert(patterns.end(), {"(abcd|abce|abde|acde|bcde)(a|bc|de)(abc|abd|abe|acd|ace|ade)(bcd|bce|bde|cde)(aab|aac|aad|aae|abb|abc|abd|abe)(de|ea|ab|bc)(abcde|abcda|abcdb|abcdc|abcde)", 
                                    "(a|aa)+b", 
                                    "(a|b)*a(a|b){7}", 
                                    "(start[A-z]{4}middle|begin[A-z]{4}end)finish", 
                                    "(start[一-鿿]{4}middle|begin[一-鿿]{4}end)finish"});

    std::vector<std::string> content;
    std::string pat;
    for (int i = 0; i < files.size(); i++) {
        content = read_tokens(std::string("examples/content/") + files[i] + ".txt");
        pat = patterns[i];

        std::cout << "\n" << files[i] << std::endl;
        report("followpos ", bench_followpos(content, pat));
        if (files[i]!="backtrack") { //this case breaks llama.cpp due to exponential complexity
            report("llama.cpp ", bench_llama(content, pat));
        }
        report("vectorscan", bench_vectorscan(content, pat));
    }
}