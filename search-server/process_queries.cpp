#include "process_queries.h"




std::vector<std::vector<Document>> ProcessQueries(const SearchServer& search_server, const std::vector<std::string_view> queries) {
    std::vector<std::vector<Document>>document_list(queries.size());
    transform(std::execution::par, queries.begin(), queries.end(), document_list.begin(), [&search_server](std::string_view local_) {return search_server.FindTopDocuments(local_); });
    return document_list;
}


std::vector<Document> ProcessQueriesJoined(const SearchServer& search_server, const std::vector<std::string_view> queries) {
    std::vector<std::vector<Document>> document_list = ProcessQueries(search_server, queries);
    std::vector<Document> result;

    for (const auto& query_documents : document_list) {
        result.insert(result.end(), query_documents.begin(), query_documents.end());
    }

    return result;
}




