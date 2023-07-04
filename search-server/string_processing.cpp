#include "string_processing.h"

std::vector<std::string_view> SplitIntoWords(const std::string_view& text) {
    std::vector<std::string_view> words;
    words.reserve(text.size() / 2);

    std::size_t wordStart = 0;
    for (std::size_t i = 0; i < text.size(); ++i) {

        if (text[i] == ' ') {
            if (i > wordStart) {
                words.emplace_back(text.substr(wordStart, i - wordStart));
            }
            wordStart = i + 1;
        }
    }

    if (wordStart < text.size()) {
        words.emplace_back(text.substr(wordStart));
    }

    return words;
}