#pragma once
#include <vector>
#include <string>
#include <iostream>
#include <iterator>
#include <regex>
#include <map>
#include <set>
#include <algorithm>
#include <boost/locale.hpp>
#include <boost/locale/conversion.hpp>
#include "DatabaseConnection.h"

// Класс для поиска по базе: нормализация запроса, разбор на слова, поиск и сортировка по релевантности
class SearchEngine
{
public:
    explicit SearchEngine(database& db)
        : db_(db), locale_(boost::locale::generator()("")) {
    }

    // Выполнить поиск и вернуть ссылки
    std::vector<std::string> search(const std::string& query);

    // Показать отладочную информацию
    void printDebugInfo() const;

private:
    database& db_;
    std::locale locale_;
    std::string originalQuery_;
    std::string cleanedQuery_;
    std::set<std::string> words_;
    std::vector<std::map<std::string, int>> resultsPerWord_;
    std::vector<std::string> wordsInOrder_;
    std::map<std::string, int> linkWeights_;

    std::string sanitizeString(const std::string& input); // Очистка строки и приведение к нижнему регистру
    void extractWords(const std::string& str);           // Разбор на слова
    void collectResults();                               // Сбор и подсчет результатов
    std::vector<std::string> sortByWeight() const;       // Сортировка по весу
};
