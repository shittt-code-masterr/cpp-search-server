#include "remove_duplicates.h"

void RemoveDuplicates(SearchServer& search_server) {

	std::map<std::set<std::string>, int> document;
	std::set<int> bad_id;
	for (int document_id : search_server) {
		std::map<std::string, double> doc;
		doc = search_server.GetWordFrequencies(document_id);
		std::set<std::string> unique_word;
		for (auto& [word, _] : doc) {
			unique_word.insert(word);
		}
		if (document.count(unique_word) != 0) {
			bad_id.insert(document_id);
			std::cout << "Found duplicate document id " << document_id << "\n";
		}
		else {
			document.insert({ unique_word, document_id });
		}
	}
	for (int id_ : bad_id) {
		search_server.RemoveDocument(id_);
	}

};
