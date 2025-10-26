#include "finder.h"


std::vector<std::string> SearchEngine::search(const std::string& query) 
{
    originalQuery_ = query;
    cleanedQuery_ = sanitizeString(originalQuery_);
    extractWords(cleanedQuery_);
    collectResults();
    return sortByWeight();
}

void SearchEngine::printDebugInfo() const 
{
    LOG_INFO("����� ��� ������:");
    for (const auto& w : words_) {
        LOG_INFO(w);
    }
    LOG_INFO("������ � ������ (� ������� �������� ����)");
    for (const auto& pair : linkWeights_) 
    {
        LOG_INFO(pair.first + ": " + std::to_string(pair.second));
    }
}

std::string SearchEngine::sanitizeString(const std::string& input) 
{
    std::string result = input;
    try 
    {
        // ���������, ������� ������� ��, ����� ���� (�������� � ���������) � ����.
        // �� ��������� ���������� ��������.
        std::regex pattern_keep_alphanumeric(R"([^
            a b c d e f g h i j k l m n o p q r s t u v w x y z
            � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � �
            � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � �
            A B C D E F G H I J K L M N O P Q R S T U V W X Y Z])");
        result = std::regex_replace(result, pattern_keep_alphanumeric, " ");

        // ��� ������������� ������� ������������ � ���� ������ "_"
        std::regex spacePattern(R"(\s+)");
        result = std::regex_replace(result, spacePattern, "_");

        // �������� ������ � ������� �������� � ������ �������� ������������.
        result = boost::locale::to_lower(result, locale_);
    }
    catch (const std::exception& ex) 
    {
        // ���� ����� ��������� ��� boost ������ ��������� � �� ��������.
        // ������ �������� ������.
        std::string er = ex.what();
        LOG_ERR("\nRegex error: " + er);
    }
    return result;
}

void SearchEngine::extractWords(const std::string& str) {
    size_t start = 0;
    for (size_t i = 0; i <= str.size(); ++i) {
        // ���� ����� ������, ���� ������ �����������
        if (i == str.size() || str[i] == '_') {
            if (i > start) {
                std::string word = str.substr(start, i - start);
                if (word.length() >= 3) {
                    // set ������������� ����� ���������
                    words_.insert(word);
                }
            }
            start = i + 1;
        }
    }
}


void SearchEngine::collectResults() 
{
    for (const auto& word : words_) 
    {
        try 
        {
            // ��������� ������ � ����: �������� map<������, ���>.
            auto result = db_.seachRequest(word);

            // ��������� �� ����������� ��� ������ �������.
            resultsPerWord_.push_back(result);
            wordsInOrder_.push_back(word);

            // ��������� ����� ��� ������ ������.
            for (const auto& entry : result) {
                linkWeights_[entry.first] += entry.second;
            }
        }
        catch (const std::exception& ex) 
        {
            // ���� ���-�� ����� �� ��� � �������� � �������� ������, �� �� ������.
            std::string er = ex.what();
            LOG_ERR(word + ">: " + er);
        }
    }
}

std::vector<std::string> SearchEngine::sortByWeight() const 
{
    // ���������� map � ������ ���, ����� ����� ���� �������������.
    std::vector<std::pair<std::string, int>> vec(linkWeights_.begin(), linkWeights_.end());

    // ������ ��������� �� �������� ������� �������� (����).
    std::sort(vec.begin(), vec.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.second > rhs.second;
        });

    // ��������� �������������� ������ ������.
    std::vector<std::string> sortedLinks;
    sortedLinks.reserve(vec.size());
    for (const auto& pair : vec) {
        sortedLinks.push_back(pair.first);
    }
    return sortedLinks;
}
