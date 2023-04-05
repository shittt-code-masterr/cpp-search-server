#pragma once
#include <set>
#include <iostream>
#include <string>

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};
struct Document {
    Document();
    Document(int id, double relevance, int rating);
    int id;
    double relevance ;
    int rating;
};

std::ostream& operator<<(std::ostream& out, const Document& document);



template <typename StringContainer>
std::set<std::string> MakeUniqueNonEmptyStrings(const StringContainer& strings) {
    std::set<std::string> non_empty_strings;
    for (const std::string& str : strings) {
        if (!str.empty()) {
            non_empty_strings.insert(str);
        }
    }
    return non_empty_strings;
}



