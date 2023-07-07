#include"search_server.h"
#include"read_input_functions.h"
#include"string_processing.h"
#include"log_duration.h"

using namespace std::string_literals;

SearchServer::SearchServer(const std::string& stop_words_text)
    : SearchServer(SearchServer(std::string_view(stop_words_text))) {
}

SearchServer::SearchServer(const std::string_view& stop_words_text)
    : SearchServer(SplitIntoWords(stop_words_text))
{

}

void SearchServer::AddDocument(int document_id, const std::string_view& document, DocumentStatus status, const std::vector<int>& ratings) {
    using namespace std::string_literals;
    if ((document_id < 0) || (documents_.count(document_id) > 0)) {
        throw  std::invalid_argument("Invalid document_id"s);
    }
    requests.emplace_back(document);
    const auto words = SplitIntoWordsNoStop(requests.back());

    const double inv_word_count = 1.0 / words.size();
    for (const std::string_view& word : words) {
        word_to_document_freqs_[word][document_id] += inv_word_count;
        word_to_document_[document_id][word] += inv_word_count;
    }
    documents_.emplace(document_id, SearchServer::DocumentData{ ComputeAverageRating(ratings), status , document});
    document_ids_.insert(document_id);
}

std::vector<Document> SearchServer::FindTopDocuments(const  std::string_view& raw_query, DocumentStatus status) const {
    return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
        return document_status == status;
        });
}

std::vector<Document> SearchServer::FindTopDocuments(const  std::string_view& raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}


int SearchServer::GetDocumentCount() const {
    return static_cast<int>(documents_.size());
}

void SearchServer::RemoveDocument(int document_id) {
    using namespace std::string_literals;
    if ((document_id < 0) || (documents_.count(document_id) == 0)) {
        return;
    }

    for (const auto& [word, _] : word_to_document_[document_id]) { // как я понимаю, это и есть слова именно удаляемого документа, все слова лежат в word_to_document_freqs_, а в данной переменной по id получаются слова документа 
        if (word_to_document_freqs_[word].count(document_id) != 0) {
            word_to_document_freqs_[word].erase(document_id);
        }
    }
    word_to_document_.erase(document_id);
    documents_.erase(document_id);
    document_ids_.erase(document_id);

}

void SearchServer::RemoveDocument(const std::execution::sequenced_policy, int document_id) {
    RemoveDocument(document_id);
}

void SearchServer::RemoveDocument(const std::execution::parallel_policy, int document_id) {

    
   // LOG_DURATION("Operation time"s);
    if ((document_id < 0) || (documents_.count(document_id) == 0)) {
        return;
    }
    std::vector<std::string_view>tmp(word_to_document_[document_id].size());


    std::transform(std::execution::par, word_to_document_[document_id].cbegin(), word_to_document_[document_id].cend(), tmp.begin(), [](const auto& word) {

        return word.first;
        });

    for_each(std::execution::par, tmp.begin(), tmp.end(), [this, document_id](const auto& word) {
        word_to_document_freqs_[word].erase(document_id);
        });

    word_to_document_.erase(document_id);
    documents_.erase(document_id);
    document_ids_.erase(document_id);

}

std::set<int>::iterator  SearchServer::begin() const { return document_ids_.begin(); }

std::set<int>::iterator  SearchServer::end() const { return document_ids_.end(); }

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::string_view& raw_query, int document_id) const {
    const auto query = ParseQuery(raw_query,true);


    if (!documents_.count(document_id))
    {
        throw  std::out_of_range("document_id out of range ");
    }
    std::vector<std::string_view> matched_words;

    for (const std::string_view& word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            return { matched_words, documents_.at(document_id).status };
            
        }
    }
    matched_words.reserve(query.plus_words.size());
    for (const std::string_view& word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.emplace_back(word);
        }
    }

    return { matched_words, documents_.at(document_id).status };
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::execution::sequenced_policy, const  std::string_view raw_query, int document_id) const {
    return MatchDocument(raw_query, document_id);
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::execution::parallel_policy, const std::string_view raw_query, int document_id) const {
    const auto query = ParseQuery(raw_query,false);

    std::vector<std::string_view> matched_words;

    if (!documents_.count(document_id))
    {
        return  { matched_words, documents_.at(document_id).status };
    }

    if (std::any_of(std::execution::par, query.minus_words.begin(), query.minus_words.end(), [this, document_id](const auto& word) {
        
        return   word_to_document_freqs_.count(word) && word_to_document_freqs_.at(word).count(document_id);
        }))
    {
        return  { matched_words, documents_.at(document_id).status };
    }

        matched_words.resize(query.plus_words.size());
        auto it = std::copy_if(std::execution::par, query.plus_words.begin(), query.plus_words.end(), matched_words.begin(), [this, document_id](const auto& word) {
           
            return word_to_document_freqs_.count(word) && word_to_document_freqs_.at(word).count(document_id);
            });

        std::sort(std::execution::par, matched_words.begin(), it);
        auto last = std::unique(std::execution::par, matched_words.begin(), it);
        matched_words.erase(last, matched_words.end());

        return { matched_words, documents_.at(document_id).status };
}

const std::map<std::string_view, double>& SearchServer::GetWordFrequencies(int document_id) const {
    static std::map<std::string_view, double> empty_map_;
    if (word_to_document_.count(document_id) != 0) {
        return word_to_document_.at(document_id);
    }
    else {

        return empty_map_;
    }
}

bool SearchServer::IsStopWord(const std::string_view& word) const {
    return count(stop_words_.begin(), stop_words_.end(), word) > 0;
}

bool SearchServer::IsValidWord(const std::string_view& word) {
    // A valid word must not contain special characters
    return std::none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
        });
}

std::vector<std::string_view> SearchServer::SplitIntoWordsNoStop(const std::string_view& text) const {
    using namespace std::string_literals;
    std::vector<std::string_view> words;

    for (const std::string_view& word : SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            throw std::invalid_argument("Word is invalid"s);
        }
        if (!IsStopWord(word)) {
            words.emplace_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    int rating_sum = std::accumulate(ratings.begin(), ratings.end(),0);
    return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(const std::string_view& text) const {
    using namespace std::string_literals;
    if (text.empty()) {
        throw std::invalid_argument("Query word is empty"s);
    }
    std::string_view word = text;
    bool is_minus = false;
    if (word[0] == '-') {
        is_minus = true;
        word = word.substr(1);
    }
    if (word.empty() || word[0] == '-' || !IsValidWord(word)) {
        throw std::invalid_argument("Query word is invalid");
    }

    return { word, is_minus, IsStopWord(word) };
}

SearchServer::Query SearchServer::ParseQuery(const std::string_view& text,bool flag) const {
    Query result;
    for (const std::string_view& word : SplitIntoWords(text)) {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.emplace_back(query_word.data);
            }
            else {
                result.plus_words.emplace_back(query_word.data);
            }
        }
    }
    if (flag) {
        std::sort(result.minus_words.begin(), result.minus_words.end());
        std::sort(result.plus_words.begin(), result.plus_words.end());
        result.minus_words.erase(std::unique(result.minus_words.begin(), result.minus_words.end()), result.minus_words.end());
        result.plus_words.erase(std::unique(result.plus_words.begin(), result.plus_words.end()), result.plus_words.end());
    }

    return result;
}

double SearchServer::ComputeWordInverseDocumentFreq(const std::string_view& word) const {
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}

void AddDocument(SearchServer& search_server, size_t document_id, const std::string_view document, DocumentStatus status, const std::vector<int>& ratings) {
    using namespace std::string_literals;
    try {
        search_server.AddDocument(document_id, document, status, ratings);
    }
    catch (const std::invalid_argument& e) {
        std::cout << "Ошибка добавления документа "s << document_id << ": "s << e.what() << std::endl;
    }
}
/*
void FindTopDocuments(const SearchServer& search_server, const std::string_view raw_query) {
    using namespace std::string_literals;

    //LOG_DURATION("Operation time"s);
    using namespace std::string_literals;
    std::cout << "Результаты поиска по запросу: "s << raw_query << std::endl;
    try {
        for (const Document& document : search_server.FindTopDocuments(raw_query)) {
            PrintDocument(document);
        }
    }
    catch (const std::invalid_argument& e) {
        std::cout << "Ошибка поиска: "s << e.what() << std::endl;
    }

}*/

void MatchDocuments(const SearchServer& search_server, std::string_view query) {
    using namespace std::string_literals;
    LOG_DURATION("Operation time"s);

    try {

        std::cout << "Матчинг документов по запросу: "s << query << std::endl;

        for (int document_id : search_server) {
            auto [words, status] = search_server.MatchDocument(query, document_id);
            PrintMatchDocumentResult(document_id, words, status);
        }
    }
    catch (const std::invalid_argument& e) {
        std::cout << "Ошибка матчинга документов на запрос "s << query << ": "s << e.what() << std::endl;
    }

}





