#include "common.h"

#include <list>

std::vector<std::string> content = {"<think>", "\\n", "Okay", ",", "the", "user", "is", "asking", "for", "the", "current", "weather", "in", "Paris", ".", "Let", "me", "break", "this", "down", ".", "First", ",", "I", "need", "to", "figure", "out", "what", "information", "they", "need", ".", "They", "probably", "want", "to", "know", "the", "current", "conditions", ",", "like", "temperature", ",", "humidity", ",", "wind", "speed", ",", "and", "maybe", "a", "brief", "description", "of", "the", "sky", ".", "But", "the", "exact", "details", "depend", "on", "the", "data", "the", "weather", "API", "provides", ".\\", "n", "\\n", "Wait", ",", "the", "user", "also", "instructed", "me", "to", "first", "think", "through", "what", "I", "need", ",", "then", "write", "a", "detailed", "explanation", "of", "the", "question", ",", "and", "finally", "call", "the", "tool", ".", "So", "I", "need", "to", "make", "sure", "I", "don", "'t", "call", "the", "tool", "until", "the", "explanation", "is", "done", ".", "Let", "me", "start", "by", "understanding", "the", "query", "thoroughly", ".", "The", "user", "is", "asking", "for", "the", "current", "weather", "in", "Paris", ".", "That", "'s", "straightforward", ",", "but", "I", "should", "consider", "possible", "nuances", ".", "For", "example", ",", "are", "they", "planning", "to", "go", "out", ",", "or", "just", "curious", "?", "Maybe", "they", "need", "to", "know", "if", "it", "'s", "a", "good", "day", "to", "travel", ".", "But", "since", "the", "query", "is", "direct", ",", "I", "should", "focus", "on", "the", "immediate", "data", ".\\", "n", "\\n", "Now", ",", "the", "detailed", "explanation", ".", "The", "user", "wants", "at", "least", "", "4", "0", "0", "words", ".", "Let", "meth", "ink", "about", "the", "structure", ".", "Start", "by", "explaining", "the", "importance", "of", "knowing", "the", "current", "weather", ".", "Mention", "how", "weather", "affects", "daily", "activities", ",", "travel", ",", "etc", ".", "Then", ",", "break", "down", "the", "components", "of", "weather", "data", ":", "temperature", ",", "precipitation", ",", "humidity", ",", "wind", ",", "visibility", ",", "UV", "index", ",", "and", "maybe", "air", "quality", ".", "Discuss", "why", "each", "of", "these", "factors", "is", "relevant", ".", "For", "example", ",", "temperature", "helps", "in", "dressing", ",", "precipitation", "affects", "travel", "plans", ",", "wind", "speed", "can", "impact", "outdoor", "activities", ".", "Also", ",", "mention", "the", "tools", "used", "to", "gather", "this", "information", ",", "like", "weather", "stations", "and", "satellites", ".", "Highlight", "the", "role", "of", "meteor", "ological", "services", "in", "providing", "accurate", "forecasts", ".", "Then", ",", "connect", "this", "to", "the", "user", "'s", "specific", "request", "about", "Paris", ".", "Talk", "about", "the", "typical", "climate", "of", "Paris", ",", "maybe", "mention", "seasonal", "variations", ".", "Explain", "that", "the", "current", "weather", "could", "be", "influenced", "by", "factors", "like", "fronts", ",", "pressure", "systems", ",", "or", "local", "geography", ".", "Em", "phas", "ize", "the", "need", "for", "real", "-time", "data", "to", "give", "an", "accurate", "answer", ".", "Finally", ",", "transition", "into", "the", "need", "to", "call", "the", "get", "_weather", "function", "to", "retrieve", "the", "latest", "information", ".", "Make", "sure", "to", "cover", "all", "aspects", "without", "using", "markdown", ",", "keeping", "it", "natural", "and", "flowing", ".", "Check", "for", "any", "missing", "elements", "and", "ensure", "the", "explanation", "is", "comprehensive", "enough", ".", "Once", "that", "'s", "done", ",", "proceed", "to", "call", "the", "function", "with", "the", "city", "parameter", "set", "to", "Paris", ".\\", "n", "</think>", "\\n", "\\n", "The", "question", "\\\"", "What", "is", "the", "weather", "in", "Paris", "right", "now", "?", "\\\"", "appears", "de", "cept", "ively", "simple", ",", "but", "it", "encompasses", "a", "complex", "inter", "play", "of", "atmospheric", "phenomena", ",", "technological", "data", "collection", ",", "and", "human", "interpretation", ".", "To", "answer", "this", ",", "one", "must", "first", "consider", "the", "multif", "ac", "eted", "nature", "of", "weather", "itself", "âĢĶa", "dynamic", "system", "influenced", "by", "global", "and", "local", "factors", ".", "Weather", "is", "not", "merely", "a", "snapshot", "of", "temperature", ";", "it", "is", "a", "sym", "phony", "of", "variables", "including", "air", "pressure", ",", "humidity", ",", "wind", "patterns", ",", "precipitation", ",", "solar", "radiation", ",", "and", "atmospheric", "stability", ".", "Each", "element", "interacts", "with", "the", "others", ",", "creating", "conditions", "that", "can", "shift", "hourly", "or", "even", "minute", "by", "minute", ".", "For", "instance", ",", "a", "seemingly", "mild", "day", "in", "Paris", "could", "transition", "to", "thunder", "storms", "if", "a", "cold", "front", "coll", "ides", "with", "warm", ",", "moist", "air", "over", "the", "Se", "ine", "River", ".", "Understanding", "this", "requires", "more", "than", "a", "curs", "ory", "glance", "at", "a", "forecast", ";", "it", "demands", "an", "appreciation", "of", "the", "underlying", "meteor", "ological", "processes", ".\\", "n", "\\n", "The", "user", "âĢĻs", "query", "also", "implies", "a", "desire", "for", "actionable", "information", ".", "Why", "does", "Paris", "matter", "?", "As", "the", "capital", "of", "France", ",", "it", "is", "a", "major", "cultural", "and", "economic", "hub", ",", "with", "millions", "of", "visitors", "annually", ".", "Travel", "ers", ",", "commuters", ",", "and", "residents", "alike", "rely", "on", "accurate", "weather", "data", "to", "plan", "activities", ",", "from", "outdoor", "events", "at", "the", "E", "iff", "el", "Tower", "to", "navigating", "the", "city", "âĢĻs", "public", "transportation", "during", "rain", ".", "Even", "locals", "might", "need", "real", "-time", "updates", "for", "daily", "routines", ",", "such", "as", "determining", "whether", "to", "carry", "an", "umbrella", "or", "adjust", "clothing", "layers", ".", "The", "stakes", "extend", "beyond", "comfort", ":", "extreme", "weather", "events", ",", "like", "the", "", "2", "0", "2", "1", "heat", "wave", "that", "broke", "records", "across", "Europe", ",", "highlight", "the", "critical", "role", "of", "timely", "weather", "insights", "in", "public", "safety", ".\\", "n", "\\n", "To", "address", "the", "question", "compreh", "ensively", ",", "one", "must", "consider", "the", "tools", "and", "technologies", "that", "gather", "this", "data", ".", "Meteor", "ological", "services", "like", "M", "Ã©tÃ©", "o", "-F", "rance", "(", "the", "French", "National", "Meteor", "ological", "Service", ")", "employ", "an", "intricate", "network", "of", "satellites", ",", "radar", "systems", ",", "weather", "balloons", ",", "and", "ground", "-based", "sensors", ".", "These", "instruments", "measure", "parameters", "such", "as", "bar", "ometric", "pressure", ",", "dew", "point", ",", "wind", "speed", ",", "and", "cloud", "cover", ".", "Advanced", "models", "then", "process", "this", "data", "to", "generate", "forecasts", ".", "However", ",", "\\\"", "current", "weather", "\\\"", "refers", "to", "observations", "at", "a", "specific", "moment", ",", "not", "predictions", ".", "This", "distinction", "is", "crucial", "âĢĶ", "real", "-time", "data", "is", "gathered", "via", "automated", "stations", "and", "remote", "sensing", ",", "ensuring", "immedi", "acy", "but", "also", "requiring", "constant", "calibration", "to", "account", "for", "measurement", "errors", "or", "environmental", "anomalies", ".\\", "n", "\\n", "Paris", "âĢĻs", "unique", "geography", "further", "complic", "ates", "the", "picture", ".", "The", "city", "âĢĻs", "elevation", ",", "proximity", "to", "the", "Atlantic", "Ocean", ",", "and", "urban", "heat", "island", "effect", "influence", "local", "weather", "patterns", ".", "For", "example", ",", "maritime", "influences", "from", "the", "English", "Channel", "can", "bring", "moisture", ",", "while", "the", "surrounding", "countryside", "may", "create", "micro", "cl", "imates", ".", "Additionally", ",", "the", "city", "âĢĻs", "dense", "infrastructure", "affects", "wind", "flow", "and", "temperature", "distribution", ".", "These", "factors", "mean", "that", "weather", "in", "Paris", "can", "differ", "significantly", "between", "the", "Ãİ", "le", "de", "France", "region", "and", "the", "outskirts", ",", "or", "even", "between", "districts", "like", "Mont", "mart", "re", "and", "La", "DÃ©", "f", "ense", ".\\", "n", "\\n", "Finally", ",", "the", "answer", "must", "balance", "technical", "accuracy", "with", "accessibility", ".", "While", "raw", "data", "is", "valuable", ",", "presenting", "it", "in", "a", "user", "-friendly", "format", "âĢĶ", "such", "as", "temperature", "in", "Celsius", ",", "wind", "speed", "in", "kilometers", "per", "hour", ",", "and", "precipitation", "likelihood", "âĢĶ", "ens", "ures", "practical", "ity", ".", "This", "is", "where", "the", "`", "get", "_weather", "`", "function", "becomes", "essential", ",", "brid", "ging", "the", "gap", "between", "complex", "meteor", "ological", "science", "and", "everyday", "utility", ".", "By", "calling", "this", "tool", ",", "we", "retrieve", "the", "latest", ",", "most", "precise", "information", "tailored", "to", "Paris", ",", "enabling", "a", "meaningful", "response", "to", "the", "user", "âĢĻs", "query", ".", "\\", "n", "\\n", "<tool_call>", "\\n", "{\\\"", "name", "\\\":", "\\\"", "get", "_weather", "\\\",", "\\\"", "arguments", "\\\":", "{", "\\\"", "city", "\\\":", "\\\"", "Paris", "\\\"", "}}", "\\", "n", "</tool_call>"};
std::string              pat     = "<tool_call>";

bool advance_dfa(DFA & dfa, size_t & current_state, char32_t character) {
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

bool advance_dfas(DFA & dfa, std::list<size_t> & live_dfas, char32_t character) {
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

bool feed(DFA & dfa, std::list<size_t> & live_dfas, std::string & tok) {
    for (char32_t character : tok) {
        if (advance_dfas(dfa, live_dfas, character)) {
            return true;
        }
    }
    return false;
}

#include <iostream>

int main() {
    DFA               dfa = regex_to_dfa(pat);
    std::list<size_t> live_dfas;

    for (std::string tok : content) {
        if (feed(dfa, live_dfas, tok)) {
            std::cout << "match found at token: " << tok << std::endl;
            break;
        }
    }
}
