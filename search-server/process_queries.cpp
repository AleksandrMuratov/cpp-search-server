#include <algorithm>
#include <execution>
#include <utility>
#include "process_queries.h"

std::vector<std::vector<Document>> ProcessQueries(
    const SearchServer& search_server,
    const std::vector<std::string>& queries) {

    std::vector<std::vector<Document>> result(queries.size());
    std::transform(std::execution::par, queries.begin(), queries.end(), result.begin(), [&search_server](const std::string& query) {return search_server.FindTopDocuments(query); });
    return result;
}

std::vector<Document> ProcessQueriesJoined(
    const SearchServer& search_server,
    const std::vector<std::string>& queries) {

    std::vector<std::vector<Document>> documents(ProcessQueries(search_server, queries));
    std::vector<Document> res;
    for (auto& vec : documents) {
        for (auto& doc : vec) {
            res.push_back(std::move(doc));
        }
    }
    return res;
}