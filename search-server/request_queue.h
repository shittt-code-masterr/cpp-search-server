#pragma once
#include "search_server.h"
#include <vector>
#include <string>
#include <deque>
class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server)
        :search_server_(search_server),
        time(0)

    {
    }

    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate);

    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);

    std::vector<Document> AddFindRequest(const std::string& raw_query);

    int GetNoResultRequests() const;

private:
    struct QueryResult {
        int time;
        std::vector<Document> result;
    };
    const SearchServer& search_server_;
    std::deque <QueryResult> requests_;
    const static int min_in_day_ = 1440;
    int time;
    // возможно, здесь вам понадобится что-то ещё
};

template <typename DocumentPredicate>
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
    if (time >= min_in_day_) {
        requests_.pop_front();
    }
    ++time;
    std::vector<Document> result = search_server_.FindTopDocuments(raw_query, document_predicate);

    if (result.empty()) {
        requests_.push_back({ time, result });
    }
    return result;
}