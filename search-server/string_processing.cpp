#include <iostream>
#include <algorithm>
#include "string_processing.h"

std::vector<std::string> SplitIntoWords(const std::string& text) {
    std::vector<std::string> words;
    std::string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        }
        else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}

std::vector<std::string_view> SplitIntoWordsView(std::string_view text) {
    std::vector<std::string_view> result;
    text.remove_prefix(std::min(text.find_first_not_of(' '), text.size()));
    while (!text.empty()) {
        size_t space = text.find(' ');
        result.push_back(text.substr(0, space));
        text.remove_prefix(std::min(space, text.size()));
        text.remove_prefix(std::min(text.find_first_not_of(' '), text.size()));
    }
    return result;
}