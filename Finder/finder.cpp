#include <vector>
#include <string>
#include <iostream>
#include <iterator>
#include <regex>
#include <boost/locale.hpp>
#include <boost/locale/conversion.hpp>
#include "../Spider/database.h"

// сортируем ссылки по весам
std::vector<std::string> findByFrequency(std::map<std::string, int>& linkWeight) {
	std::vector<std::string> seachResults;

	// создаем вектор пар из map чтобы удобнее сортировать
	std::vector<std::pair<std::string, int>> vec(linkWeight.begin(), linkWeight.end());

	// сортируем по убыванию частоты
	std::sort(vec.begin(), vec.end(), [](const auto& lhs, const auto& rhs) {
		return lhs.second > rhs.second;
		});

	// заполняем результирующий вектор только ссылками
	for (const auto& pair : vec) {
		seachResults.push_back(pair.first);
	}

	return seachResults;
}

// основная функция поиска
std::vector<std::string> finder(std::string inSeachString, database& DB) {
	std::vector<std::string> seachResults;

	// оставляем только буквы и цифры, остальные символы заменяем пробелом
	try
	{
		std::regex pattern_keep_alphanumeric(R"([^a-zа-яёA-ZА-ЯЁ0-9])");
		inSeachString = std::regex_replace(inSeachString, pattern_keep_alphanumeric, " ");

		// лишние пробелы заменяем на "_"
		std::regex SPACEpattern(R"(\s+)");
		inSeachString = std::regex_replace(inSeachString, SPACEpattern, "_");
	}
	catch (const std::exception& ex) {
		std::cout << __FILE__ << ", line: " << __LINE__ << std::endl;
		std::cout << "Ошибка регулярки: " << ex.what() << std::endl;
	}

	// приводим все к нижнему регистру
	boost::locale::generator gen;
	std::locale loc = gen("");
	inSeachString = boost::locale::to_lower(inSeachString, loc);

	// разбиваем строку на слова и сохраняем в set, чтобы уникальные
	std::set<std::string> setInWords;
	unsigned int cut_end_pos{ 0 };
	unsigned int cut_start_pos{ 0 };
	unsigned int stringLength = inSeachString.length();
	for (unsigned int iter = 0; iter < stringLength; ++iter) {
		if (inSeachString[iter] == '_') {
			cut_end_pos = iter;
			std::string word = inSeachString.substr(cut_start_pos, cut_end_pos - cut_start_pos);
			if (word.length() >= 3) {
				setInWords.insert(word);
			}
			cut_start_pos = iter + 1;
		}
		else if (iter == (stringLength - 1)) {
			cut_end_pos = stringLength;
			std::string word = inSeachString.substr(cut_start_pos, cut_end_pos - cut_start_pos);
			if (word.length() >= 3) {
				setInWords.insert(word);
			}
		}
	}
	// выводим слова для проверки
	std::cout << "\nСлова для поиска: \n";
	for (const auto& word : setInWords) {
		std::cout << word << std::endl;
	}

	// результаты поиска для каждого слова
	std::vector<std::map<std::string, int>> resultsPerWord;
	// слова в порядке обработки
	std::vector<std::string> wordsInOrder;
	// веса ссылок
	std::map<std::string, int> linkWeight;

	// получаем результаты поиска по каждому слову из базы
	try {
		for (const auto& word : setInWords)
		{
			try {
				resultsPerWord.push_back(DB.seachRequest(word));
				wordsInOrder.push_back(word);
			}
			catch (const std::exception& ex) {
				std::cout << __FILE__ << ", line: " << __LINE__ << std::endl;
				std::cout << "Не получилось найти <" + word + "> в базе: " << ex.what() << std::endl;
			}
		}
	}
	catch (const std::exception& ex) {
		std::cout << "Ошибка соединения с базой: " << ex.what() << std::endl;
	}

	// выводим результаты поиска по словам
	std::cout << "\nРезультаты поиска для слов: \n";
	unsigned int wordIter = 0;
	for (const auto& vectors : resultsPerWord) {
		std::cout << wordsInOrder[wordIter] << ": \n";
		for (const auto& pair : vectors) {
			std::cout << pair.first << ": " << pair.second << std::endl;
			// считаем вес ссылок
			linkWeight[pair.first] += pair.second;
		}
		++wordIter;
	}

	std::cout << "\nСсылки с весами (по убыванию веса)\n";

	// сортируем ссылки по весу
	std::vector<std::string> sortedLinks = findByFrequency(linkWeight);

	// выводим отсортированные ссылки
	for (const auto& link : sortedLinks) {
		std::cout << link << ": " << linkWeight[link] << std::endl;
	}

	// возвращаем результат
	seachResults = std::move(sortedLinks);
	return seachResults;
}
