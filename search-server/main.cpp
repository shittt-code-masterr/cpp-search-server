#include <algorithm>
#include <cmath>
#include <map>
#include <string>
#include <set>
#include <utility>
#include <numeric>
#include <vector>
#include <iostream>
#include <cassert>
//#include "search_server.h"


using namespace std;
const int MAX_RESULT_DOCUMENT_COUNT = 5;
const auto EPSILON = 1e-6;
// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов

//тавьте вашу реализацию класса SearchServer сюда */
template <typename T>
void RunTestImpl(T& func, const string& fun_name) {
    func();
    cerr << fun_name << " OK"s << endl;
    /* Напишите недостающий код */
}
#define RUN_TEST(func) RunTestImpl((func), #func)

template <typename T>
ostream& operator<<(ostream& os, const vector<T>& v) {
    bool flag = true;
    for (auto& i : v) {
        if (flag) {
            os << "[" << i;
            flag = false;
            continue;
        }
        os << ", " << i;
    }
    os << "]";
    return os;
}
template <typename T>
ostream& operator<<(ostream& os, const set<T>& s) {
    bool flag = true;
    for (auto& i : s) {
        if (flag) {
            os << "{" << i;
            flag = false;
            continue;
        }
        os << ", " << i;
    }
    os << "}";
    return os;
}

template <typename T, typename U>
ostream& operator<<(ostream& os, const map<T, U>& m) {
    bool flag = true;
    for (auto& [key, value] : m) {
        if (flag) {
            os << "{" << key << ": "s << value;
            flag = false;
            continue;
        }
        os << ", " << key << ": "s << value;
    }
    os << "}";
    return os;
}


template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file,
    const string& func, unsigned line, const string& hint) {
    if (t != u) {
        cout << boolalpha;
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cout << t << " != "s << u << "."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertImpl(bool value, const string& expr_str, const string& file, const string& func, unsigned line,
    const string& hint) {
    if (!value) {
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))
/*

   Подставьте сюда вашу реализацию макросов
   ASSERT, ASSERT_EQUAL, ASSERT_EQUAL_HINT, ASSERT_HINT и RUN_TEST
*/

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        }
        else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}

struct Document {
    int id;
    double relevance;
    int rating;
};

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

class SearchServer {
public:
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document, DocumentStatus status,
        const vector<int>& ratings) {
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
    }
    // ---------------------------------------------------------------------------------------------- 1 parameters

    vector<Document> FindTopDocuments(const string& raw_query) const {
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);

    }
    // ----------------------------------------------------------------------------------------------

    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status) const {
        return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus doc_status, int rating) {
            return doc_status == static_cast<DocumentStatus>(status);
            });

    }
    // ----------------------------------------------------------------------------------------------
    template <typename Lambda>
    vector<Document> FindTopDocuments(const string& raw_query, const Lambda& lambda) const {

        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, lambda);

        sort(matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs) {
                if (abs(lhs.relevance - rhs.relevance) < EPSILON) {
                    return lhs.rating > rhs.rating;
                }
                else {
                    return lhs.relevance > rhs.relevance;
                }
            });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }
    // ----------------------------------------------------------------------------------------------

    int GetDocumentCount() const {
        return static_cast<int>(documents_.size());
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query,
        int document_id) const {
        const Query query = ParseQuery(raw_query);
        vector<string> matched_words;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }
        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.clear();
                break;
            }
        }
        return { matched_words, documents_.at(document_id).status };
    }

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    static int ComputeAverageRating(const vector<int>& ratings) {
        if (ratings.empty()) {
            return 0;
        }
       
        return accumulate(ratings.begin(), ratings.end(), 0) / static_cast<int>(ratings.size());
    }

    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(string text) const {
        bool is_minus = false;
        // Word shouldn't be empty
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        return { text, is_minus, IsStopWord(text) };
    }

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    Query ParseQuery(const string& text) const {
        Query query;
        for (const string& word : SplitIntoWords(text)) {
            const QueryWord query_word = ParseQueryWord(word);
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    query.minus_words.insert(query_word.data);
                }
                else {
                    query.plus_words.insert(query_word.data);
                }
            }
        }
        return query;
    }

    // Existence required
    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }

    // ----------------------------------------------------------------------------------------------


// ----------------------------------------------------------------------------------------------
    template <typename Lambda>
    vector<Document> FindAllDocuments(const Query& query, const Lambda& lambda) const {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                bool lam = lambda(document_id, documents_.at(document_id).status, documents_.at(document_id).rating);
                if (lam) {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }

        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back(
                { document_id, relevance, documents_.at(document_id).rating });
        }
        return matched_documents;
    }
};
// -------- Начало модульных тестов поисковой системы ----------

// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };
    // Сначала убеждаемся, что поиск слова, не входящего в список стоп-слов,
    // находит нужный документ
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT(found_docs.size() == 1);
        const Document& doc0 = found_docs[0];
        ASSERT(doc0.id == doc_id);
    }

    // Затем убеждаемся, что поиск этого же слова, входящего в список стоп-слов,
    // возвращает пустой результат
    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT(server.FindTopDocuments("in"s).empty());
    }
    
}

/*
Разместите код остальных тестов здесь
*/
void TestAddDocument() {// тест добавления документов
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("cat city"s);
        ASSERT(found_docs.size() == 1);
        const Document& doc0 = found_docs[0];
        ASSERT(doc0.id == doc_id);
    }
   
}
void TestMinWords() {// тест минус слов
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };

    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("cat -city"s);
        ASSERT(found_docs.empty());

    }
    const int doc_id_2 = 43;
    const string content_2 = "cat litle cat"s;
    const vector<int> ratings_2 = { 1, 2, 3 };
    {   SearchServer server;
    server.AddDocument(doc_id_2, content_2, DocumentStatus::ACTUAL, ratings_2);
    const auto found_docs = server.FindTopDocuments("cat -city"s);
    ASSERT(found_docs.size() == 1);
    }
    
}
void TestMatchDocument() {
    SearchServer server;
    server.AddDocument(1, "dog walks on street", DocumentStatus::ACTUAL, { 1,2,3 });
    server.AddDocument(2, "cat walks around", DocumentStatus::ACTUAL, { 3,4,5 });
    server.AddDocument(3, "cat walks on street", DocumentStatus::ACTUAL, { 5,6,7 });
    {
        const auto [matched, status] = server.MatchDocument("-cat", 2);
        ASSERT(matched.empty());
    }
    {
        const auto [matched, status] = server.MatchDocument("cat", 2);
        ASSERT(matched[0] == "cat");
    }
  
}
void TestRating() {
    SearchServer server;
    server.AddDocument(1, "dog walks on street", DocumentStatus::ACTUAL, { 1,2,3 });
    server.AddDocument(2, "cat walks around", DocumentStatus::ACTUAL, { 3,4,5 });
    {
        const auto doc = server.FindTopDocuments("cat");
        ASSERT(doc[0].rating == (3 + 4 + 5) / 3);
    }
    {
        const auto doc = server.FindTopDocuments("dog");
        ASSERT(doc[0].rating == (1 + 2 + 3) / 3);
    }
    
}
void TestRelevance() {
    SearchServer server;
    server.AddDocument(1, "dog walks on street", DocumentStatus::ACTUAL, { 1,2,3 });
    server.AddDocument(2, "cat walks around", DocumentStatus::ACTUAL, { 3,4,5 });



    {
        const auto doc = server.FindTopDocuments("cat walks");
        double rel = log(2) / 3.0;
        ASSERT(doc[0].relevance == rel);
        ASSERT(doc[1].relevance == 0);

    }

    
}

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST (TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST (TestAddDocument);
    RUN_TEST (TestMinWords);
    RUN_TEST(TestMatchDocument);
    RUN_TEST(TestRating);
    RUN_TEST(TestRelevance);
    
    // Не забудьте вызывать остальные тесты здесь
}

/*
Разместите код остальных тестов здесь
*/

// Функция TestSearchServer является точкой входа для запуска тестов


// --------- Окончание модульных тестов поисковой системы -----------

int main() {
    RUN_TEST(TestSearchServer);
    // Если вы видите эту строку, значит все тесты прошли успешно
    cout << "Search server testing finished"s << endl;
}
