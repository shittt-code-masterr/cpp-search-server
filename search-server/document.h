#pragma once
#include <set>
#include <iostream>
#include <string>
#include <vector>

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
    double relevance;
    int rating;
};

std::ostream& operator<<(std::ostream& out, const Document& document);

//void PrintDocument(const Document& document);

void PrintMatchDocumentResult(int document_id, const std::vector<std::string_view>& words, DocumentStatus status);



