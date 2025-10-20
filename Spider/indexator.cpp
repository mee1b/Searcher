#include "indexator.h"
#include <iostream>
#include <string.h>
#include <set>
#include <map>
#include <exception>
#include "database.h"
#include <regex>
#include <iterator>
#include <vector>
#include "HTTPclient.h"
#include "ParcerHTML.h"


std::set<std::string> indexator(database& DB, std::string inLink) {

	std::set<std::string> Links;           // Набор ссылок, найденных на странице
	std::map<std::string, int> Frequencies; // Частоты слов на странице
	int link_id;                            // ID страницы в базе данных
	std::map<std::string, int> WordIdPair; // Слово -> ID слова в базе
	std::set<std::string> wordsInDB;       // Слова, уже имеющиеся в базе
	std::set<std::string> wordsInPage;     // Слова, найденные на странице
	std::string host;                       // Домен хоста
	std::string target;                     // Ресурс на хосте (URL path)
	bool isHTTPS = false;                   // Флаг HTTPS

	// Результат индексирования: (страница, ссылки, частоты слов)
	std::tuple<std::string, std::set<std::string>, std::map<std::string, int>> indexatorResult;

	// Определяем протокол по префиксу URL
	const std::string http_pref = "http://";
	const std::string https_pref = "https://";

	// Обрезаем якорь (#) из URL
	auto grid_ptr = inLink.find('#');
	if (grid_ptr != std::string::npos)
		inLink = inLink.substr(0, grid_ptr);

	// Определяем тип сервера
	if (inLink.compare(0, https_pref.length(), https_pref) == 0) {
		isHTTPS = true;
	}
	else if (inLink.compare(0, http_pref.length(), http_pref) == 0) {
		isHTTPS = false;
	}
	else {
		host = inLink;
	}

	// Убираем префикс протокола из URL
	std::regex pattern_https(https_pref);
	std::regex pattern_http(http_pref);
	host = (isHTTPS) ? std::regex_replace(inLink, pattern_https, "") : std::regex_replace(inLink, pattern_http, "");

	// Разделяем URL на host и target
	size_t slashPos = host.find("/");
	if (slashPos != std::string::npos) {
		std::string temp_str = host;
		host = host.substr(0, slashPos);

		// Убираем параметры после "?" из target
		size_t questionMarkPos = temp_str.find("?");
		if (questionMarkPos != std::string::npos)
			temp_str = temp_str.substr(0, questionMarkPos);

		target = temp_str.substr(slashPos);
	}
	else {
		target = "/";
	}

	try {
		HTTPclient client;         // HTTP клиент для загрузки страницы
		std::string response = ""; // HTML страницы

		// Выполняем запрос по соответствующему протоколу
		if (isHTTPS)
			client.performGetRequest(host, "443", target, 11);
		else
			client.performGetRequest(host, "80", target, 11);

		response = client.getData();

		try {
			// Добавляем протокол к host для корректного парсинга
			host = (isHTTPS) ? https_pref + host : http_pref + host;

			ParcerHTML parcerHTML(response, host); // Парсер HTML
			Links = parcerHTML.getLinks();         // Извлекаем ссылки
			Frequencies = parcerHTML.getFrequencies(); // Извлекаем частоты слов

			// Добавляем адрес страницы в базу (если отсутствует)
			DB.link_add(inLink);

			// Получаем ID страницы из базы
			link_id = DB.getLinkId(inLink);

			// Получаем все слова и их ID из базы
			WordIdPair = DB.getWordId();
			bool isNewWordAdd = false;

			// Проверяем новые слова и добавляем их в базу
			for (const auto& pair : Frequencies) {
				std::string wordInPage = pair.first;
				if (WordIdPair[wordInPage] == 0) {
					isNewWordAdd = true;
					DB.word_add(wordInPage);
				}
			}

			// Если были добавлены новые слова, обновляем таблицу слов
			if (isNewWordAdd)
				WordIdPair = DB.getWordId();

			// Добавляем частоты слов для текущей страницы
			for (const auto& pair : Frequencies) {
				int wordFrequency = pair.second;
				int wordId = WordIdPair[pair.first];
				DB.frequency_add(link_id, wordId, wordFrequency);
			}

			// Формируем результат индексирования
			indexatorResult = std::make_tuple(inLink, Links, Frequencies);
		}
		catch (const std::exception& ex) {
			std::cout << "\n Fail to parse page " << inLink << ": " << ex.what();
			return Links;
		}
	}
	catch (const std::exception& ex) {
		std::cout << "\n Fail to load page " << inLink << ": " << ex.what();
		return Links;
	}

	return Links; // Возвращаем набор ссылок, найденных на странице
};
