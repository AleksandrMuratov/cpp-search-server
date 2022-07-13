#include <iostream>
#include <cmath>
#include <numeric>
#include <iterator>
#include <cassert>
#include "search_server.h"

SearchServer::SearchServer(std::string_view stop_words_text)
    : SearchServer(SplitIntoWordsView(stop_words_text))
{
}

SearchServer::SearchServer(const std::string& stop_words_text)
    : SearchServer(SplitIntoWordsView(std::string_view(stop_words_text)))
{
}

void SearchServer::AddDocument(int document_id, std::string_view document, DocumentStatus status, const std::vector<int>& ratings) {
    using namespace std::literals;
    if ((document_id < 0) || (documents_.count(document_id) > 0)) {
        throw std::invalid_argument("Invalid document_id"s);
    }
    const auto words = SplitIntoWordsNoStop(document);

    const double inv_word_count = 1.0 / words.size();
    auto& word_frequencies = document_id_to_word_frequencies_[document_id];
    for (std::string_view word : words) {
        word_to_document_freqs_[std::string(word)][document_id] += inv_word_count;
        word_frequencies[word_to_document_freqs_.find(std::string(word))->first] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
    document_ids_.insert(document_id);
}

std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
        return document_status == status;
        });
}

std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const {
    return documents_.size();
}

const std::map<std::string_view, double>& SearchServer::GetWordFrequencies(int document_id) const {
    auto it = document_id_to_word_frequencies_.find(document_id);
    assert(it != document_id_to_word_frequencies_.end());
    return it->second;
}

void SearchServer::RemoveDocument(int document_id) {
    if (document_ids_.count(document_id)) {
        document_ids_.erase(document_id);
        documents_.erase(document_id);
        const auto& word_frequencies = document_id_to_word_frequencies_[document_id];
        std::vector<std::map<std::string, std::map<int, double>>::iterator> its_for_erase;
        for (const auto& [word, freq] : word_frequencies) {
            auto it = word_to_document_freqs_.find(word);
            it->second.erase(document_id);
            if (it->second.empty()) {
                word_to_document_freqs_.erase(it);
            }
        }
        document_id_to_word_frequencies_.erase(document_id);
    }
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(std::string_view raw_query, int document_id) const {
    using namespace std::string_literals;
    if (!document_ids_.count(document_id)) {
        throw std::out_of_range("Id of document is not valid");
    }
    const auto query = ParseQuery(raw_query);
    if (std::any_of(query.minus_words.begin(), query.minus_words.end(), [this, document_id](std::string_view word) {
        auto it = word_to_document_freqs_.find(word);
        return it != word_to_document_freqs_.end() && it->second.count(document_id);
        })) {
        return { {}, documents_.at(document_id).status };
    }
    std::vector<std::string_view> matched_words(query.plus_words.size());
    auto it_for_erase = std::copy_if(query.plus_words.begin(), query.plus_words.end(), matched_words.begin(), [this, document_id](std::string_view word) {
        auto it = word_to_document_freqs_.find(word);
        return it != word_to_document_freqs_.end() && it->second.count(document_id);
        });
    matched_words.resize(it_for_erase - matched_words.begin());
    return { matched_words, documents_.at(document_id).status };
}

bool SearchServer::IsStopWord(std::string_view word) const {
    return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(std::string_view word) {
    // A valid word must not contain special characters
    return std::none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
        });
}

std::vector<std::string_view> SearchServer::SplitIntoWordsNoStop(std::string_view text) const {
    using namespace std::literals;
    std::vector<std::string_view> words;
    for (std::string_view word : SplitIntoWordsView(text)) {
        if (!IsValidWord(word)) {
            throw std::invalid_argument("Word "s + std::string(word) + " is invalid"s);
        }
        if (!IsStopWord(std::string(word))) {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    return std::accumulate(ratings.begin(), ratings.end(), 0) / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string_view text) const {
    using namespace std::literals;
    if (text.empty()) {
        throw std::invalid_argument("Query word is empty"s);
    }
    bool is_minus = false;
    if (text[0] == '-') {
        is_minus = true;
        text.remove_prefix(1);
    }
    if (text.empty() || text[0] == '-' || !IsValidWord(text)) {
        throw std::invalid_argument("Query word "s + std::string(text) + " is invalid");
    }

    return { text, is_minus, IsStopWord(text) };
}

SearchServer::Query SearchServer::ParseQuery(std::string_view text, bool is_sort_and_unique) const {
    std::vector<std::string_view> plus_words, minus_words;
    for (std::string_view word : SplitIntoWordsView(text)) {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                minus_words.push_back(query_word.data);
            }
            else {
                plus_words.push_back(query_word.data);
            }
        }
    }
    if (is_sort_and_unique) {
        std::sort(plus_words.begin(), plus_words.end());
        auto it_end_for_plus_words = std::unique(plus_words.begin(), plus_words.end());
        plus_words.resize(it_end_for_plus_words - plus_words.begin());
        std::sort(minus_words.begin(), minus_words.end());
        auto it_end_for_minus_words = std::unique(minus_words.begin(), minus_words.end());
        minus_words.resize(it_end_for_minus_words - minus_words.begin());
    }
    return {move(plus_words), move(minus_words)};
}

double SearchServer::ComputeWordInverseDocumentFreq(std::string_view word) const {
    return std::log(GetDocumentCount() * 1.0 / word_to_document_freqs_.find(word)->second.size());
}