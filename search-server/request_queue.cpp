#include"request_queue.h"

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentStatus status) {
    if (time >= min_in_day_) {
        requests_.pop_front();
    }
    ++time;
    std::vector<Document> result = search_server_.FindTopDocuments(raw_query, status);

    if (result.empty()) {
        requests_.push_back({ time, result });
    }
    return result;
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query) {
    if (time >= min_in_day_) {
        requests_.pop_front();
    }
    ++time;
    std::vector<Document> result = search_server_.FindTopDocuments(raw_query);

    if (result.empty()) {
        requests_.push_back({ time, result });
    }
    return result;
}

int RequestQueue::GetNoResultRequests() const {
    return static_cast<int>(requests_.size());

}