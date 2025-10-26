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
    LOG_INFO("Слова для поиска:");
    for (const auto& w : words_) {
        LOG_INFO(w);
    }
    LOG_INFO("Ссылки с весами (в порядке убывания веса)");
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
        // Регулярка, которая убирает всё, кроме букв (латиница и кириллица) и цифр.
        // Всё остальное заменяется пробелом.
        std::regex pattern_keep_alphanumeric(R"([^
            a b c d e f g h i j k l m n o p q r s t u v w x y z
            а б в г д е ё ж з и й к л м н о п р с т у ф х ц ч ш щ ъ ы ь э ю я
            А Б В Г Д Е Ё Ж З И Й К Л М Н О П Р С Т У Ф Х Ц Ч Ш Щ Ъ Ы Ь Э Ю Я
            A B C D E F G H I J K L M N O P Q R S T U V W X Y Z])");
        result = std::regex_replace(result, pattern_keep_alphanumeric, " ");

        // Все повторяющиеся пробелы схлопываются в один символ "_"
        std::regex spacePattern(R"(\s+)");
        result = std::regex_replace(result, spacePattern, "_");

        // Приводим строку к нижнему регистру с учётом языковых особенностей.
        result = boost::locale::to_lower(result, locale_);
    }
    catch (const std::exception& ex) 
    {
        // Если вдруг регулярка или boost решили сломаться — не паникуем.
        // Просто логируем ошибку.
        std::string er = ex.what();
        LOG_ERR("\nRegex error: " + er);
    }
    return result;
}

void SearchEngine::extractWords(const std::string& str) {
    size_t start = 0;
    for (size_t i = 0; i <= str.size(); ++i) {
        // либо конец строки, либо найден разделитель
        if (i == str.size() || str[i] == '_') {
            if (i > start) {
                std::string word = str.substr(start, i - start);
                if (word.length() >= 3) {
                    // set автоматически уберёт дубликаты
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
            // Выполняем запрос к базе: получаем map<ссылка, вес>.
            auto result = db_.seachRequest(word);

            // Сохраняем по отдельности для каждой лексемы.
            resultsPerWord_.push_back(result);
            wordsInOrder_.push_back(word);

            // Обновляем общий вес каждой ссылки.
            for (const auto& entry : result) {
                linkWeights_[entry.first] += entry.second;
            }
        }
        catch (const std::exception& ex) 
        {
            // Если что-то пошло не так с запросом — логируем ошибку, но не падаем.
            std::string er = ex.what();
            LOG_ERR(word + ">: " + er);
        }
    }
}

std::vector<std::string> SearchEngine::sortByWeight() const 
{
    // Превращаем map в вектор пар, чтобы можно было отсортировать.
    std::vector<std::pair<std::string, int>> vec(linkWeights_.begin(), linkWeights_.end());

    // Лямбда сортирует по убыванию второго элемента (веса).
    std::sort(vec.begin(), vec.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.second > rhs.second;
        });

    // Формируем результирующий список ссылок.
    std::vector<std::string> sortedLinks;
    sortedLinks.reserve(vec.size());
    for (const auto& pair : vec) {
        sortedLinks.push_back(pair.first);
    }
    return sortedLinks;
}
