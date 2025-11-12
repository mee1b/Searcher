#pragma once
#include <vector>
#include <string>
#include <map>
#include <set>
#include "gumbo.h"

class ParcerHTML {
private:
	std::string Line; // Весь текст страницы одной строкой, слова разделены "_"
	std::vector<std::string> Words; // Все слова длиной >3 символов
	std::set<std::string> Links; // Ссылки, найденные на странице
	std::map<std::string, int> Frequencies; // Частоты слов на странице (слово -> количество)

	// Рекурсивно ищет ссылки в дереве HTML
	void findLinks(GumboNode* node, const std::string& SourceLink);

public:
	// Конструктор: принимает HTML-код страницы и адрес сайта
	ParcerHTML(std::string HTML_strings, std::string SourceHost);

	//Валидация ссылки
	bool isValidLink(const std::string& link);

	// Получить ссылки
	std::set<std::string> getLinks();

	// Получить очищенный текст страницы
	std::string getLine();

	// Получить все слова
	std::vector<std::string> getWords();

	// Получить частоты слов
	std::map<std::string, int> getFrequencies();
};
