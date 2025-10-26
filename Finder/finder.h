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

// ����� ��� ������ �� ����: ������������ �������, ������ �� �����, ����� � ���������� �� �������������
class SearchEngine
{
public:
    explicit SearchEngine(database& db)
        : db_(db), locale_(boost::locale::generator()("")) {
    }

    // ��������� ����� � ������� ������
    std::vector<std::string> search(const std::string& query);

    // �������� ���������� ����������
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

    std::string sanitizeString(const std::string& input); // ������� ������ � ���������� � ������� ��������
    void extractWords(const std::string& str);           // ������ �� �����
    void collectResults();                               // ���� � ������� �����������
    std::vector<std::string> sortByWeight() const;       // ���������� �� ����
};
