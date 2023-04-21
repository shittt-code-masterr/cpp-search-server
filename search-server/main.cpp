﻿#include"search_server.h"
#include"document.h"
#include"read_input_function.h"
#include"request_queue.h"
#include"paginator.h"
#include"string_processing.h"
#include "remove_duplicates.h"

using namespace std;


int main() {
    setlocale(LC_ALL, "Russian");
    SearchServer search_server("and with"s);
    search_server.AddDocument(1, "funny pet and nasty rat"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    search_server.AddDocument(6, "funny pet and nasty rat"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    search_server.AddDocument(7, "funny pet and nasty rat"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    search_server.AddDocument(8, "funny pet and nasty rat"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    search_server.AddDocument(9, "funny pet and nasty rat"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    search_server.AddDocument(2, "funny pet with curly hair"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    search_server.AddDocument(3, "big cat nasty hair"s, DocumentStatus::ACTUAL, { 1, 2, 8 });
    search_server.AddDocument(4, "big dog cat Vladislav"s, DocumentStatus::ACTUAL, { 1, 3, 2 });
    search_server.AddDocument(5, "big dog hamster Borya"s, DocumentStatus::ACTUAL, { 1, 1, 1 });
    MatchDocuments(search_server, "big"s);
    FindTopDocuments(search_server, "cat"s);
    const auto search_results = search_server.FindTopDocuments("curly cat   dog pet"s);
    int page_size = 2;
    const auto pages = Paginate(search_results, page_size);
    // Выводим найденные документы по страницам
    for (auto page = pages.begin(); page != pages.end(); ++page) {
        cout << *page << endl;
        cout << "Page break"s << endl;
    }
    for (const int document_id : search_server) {
        cout << document_id;
    }
    RemoveDuplicates(search_server);

    for (const int document_id : search_server) {
        cout << document_id;
    }

}