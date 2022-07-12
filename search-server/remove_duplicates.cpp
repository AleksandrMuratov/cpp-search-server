#include <set>
#include <string>
#include <string_view>
#include <iostream>
#include <vector>
#include "remove_duplicates.h"

void RemoveDuplicates(SearchServer& search_server) {
	std::set<std::set<std::string_view>> words_of_documents;
	std::vector<int> document_id_to_erase;
	for(int document_id : search_server) {
		const std::map<std::string_view, double>&  words_freq = search_server.GetWordFrequencies(document_id);
		std::set<std::string_view> words_of_document;
		for (const auto& [word, freq] : words_freq) {
			words_of_document.insert(word);
		}
		if (!words_of_documents.insert(words_of_document).second) {
			std::cout << "Found duplicate document id " << document_id << '\n';
			document_id_to_erase.push_back(document_id);
		}
	}
	for (int document_id : document_id_to_erase) {
		search_server.RemoveDocument(document_id);
	}
}