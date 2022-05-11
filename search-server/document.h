#pragma once
#include <iostream>
#include <vector>

struct Document {
    Document();

    Document(int id, double relevance, int rating);

    int id;
    double relevance;
    int rating;
};

std::ostream& operator<<(std::ostream& out, const Document& document);

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

void PrintDocument(const Document& document);

void PrintMatchDocumentResult(int document_id, const std::vector<std::string>& words, DocumentStatus status);