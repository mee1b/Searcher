#include "ParcerHTML.h"

#include <vector>
#include <string>
#include <iostream>
#include <iterator>
#include <regex>
#include <boost/locale.hpp>
#include <boost/locale/conversion.hpp>
#include "gumbo.h"

#define STRINGIZE(x) #x
#define TO_STRING(x) STRINGIZE(x)

ParcerHTML::ParcerHTML(std::string HTML_strings, std::string SourceHost) {
    // Проверяем, что входная строка не пустая
    if (HTML_strings.length() == 0) {
        throw std::domain_error(std::string(__FILE__) + ": no strings in input vector: " + std::string(TO_STRING(HTML_strings)));
    }

    // Парсим HTML с помощью Gumbo
    GumboOutput* output = gumbo_parse(HTML_strings.c_str());
    // Рекурсивно ищем все ссылки на странице
    findLinks(output->root, SourceHost);
    gumbo_destroy_output(&kGumboDefaultOptions, output);

    // Удаляем все HTML-теги и комментарии
    std::regex tagRegex(R"((<[^>]*>|<!--[^>]*-->))");
    Line = std::regex_replace(HTML_strings, tagRegex, " ");

    // Удаляем символы &nbsp;
    std::regex pattern_nbsp(R"((&nbsp;\s*)+)");
    Line = std::regex_replace(Line, pattern_nbsp, " ");

    // Оставляем только буквы и цифры
    std::regex pattern_keep_alphanumeric(R"([^
 a b c d e f g h i j k l m n o p q r s t u v w x y z
 а б в г д е ё ж з и й к л м н о п р с т у ф х ц ч ш щ ъ ы ь э ю я
 А Б В Г Д Е Ё Ж З И Й К Л М Н О П Р С Т У Ф Х Ц Ч Ш Щ Ъ Ы Ь Э Ю Я
 A B C D E F G H I J K L M N O P Q R S T U V W X Y Z])");
    Line = std::regex_replace(Line, pattern_keep_alphanumeric, " ");

    // Переводим строку в нижний регистр с учетом локали
    boost::locale::generator gen;
    std::locale loc = gen("");
    Line = boost::locale::to_lower(Line, loc);

    // Заменяем все последовательности пробелов на один символ "_"
    std::regex SPACEpattern(R"(\s+)");
    Line = std::regex_replace(Line, SPACEpattern, "_");

    // Разбиваем строку на слова и подсчитываем частоты
    unsigned int lineLength = Line.length();
    unsigned int cut_end_pos{ 0 };
    unsigned int cut_start_pos{ 0 };
    for (unsigned int iter = 0; iter < lineLength; ++iter) {
        if (Line[iter] == '_') {
            cut_end_pos = iter;
            std::string word = Line.substr(cut_start_pos, cut_end_pos - cut_start_pos);
            if ((word.length() >= 3) && (word.length() <= 40)) {
                Frequencies[word]++;
            }
            cut_start_pos = iter + 1;
        }
        else if (iter == (lineLength - 1)) { // Обработка последнего слова
            cut_end_pos = lineLength;
            std::string word = Line.substr(cut_start_pos, cut_end_pos - cut_start_pos);
            if ((word.length() >= 3) && (word.length() <= 40)) {
                Frequencies[word]++;
            }
        }
    }
}

// Возвращаем множество ссылок
std::set<std::string> ParcerHTML::getLinks() { return Links; }

// Возвращаем "очищенную" строку страницы
std::string ParcerHTML::getLine() { return Line; }

// Возвращаем вектор слов (если нужен)
std::vector<std::string> ParcerHTML::getWords() { return Words; }

// Возвращаем частоты слов
std::map<std::string, int> ParcerHTML::getFrequencies() { return Frequencies; }

// Рекурсивный поиск ссылок на странице
void ParcerHTML::findLinks(GumboNode* node, const std::string& SourceHost) {
    if (node->type == GUMBO_NODE_ELEMENT) {
        GumboAttribute* href;
        if ((href = gumbo_get_attribute(&node->v.element.attributes, "href"))) {
            std::string link = href->value;
            if (!link.empty()) {
                // Определяем, является ли ссылка внешней или внутренней
                std::string const http_pref = "http://";
                std::string const https_pref = "https://";

                bool isHTTP = false, isHTTPS = false;
                if (link.compare(0, http_pref.length(), http_pref) == 0) isHTTP = true;
                if (link.compare(0, https_pref.length(), https_pref) == 0) isHTTPS = true;

                // Если ссылка внутренняя, добавляем полный путь
                if (!isHTTP && !isHTTPS) {
                    if (link[0] == '/') link = SourceHost + link;
                    else link = SourceHost + "/" + link;
                }

                // Убираем завершающий слеш
                std::regex pattern_slash(R"(/$)");
                link = std::regex_replace(link, pattern_slash, "");

                // Игнорируем ссылки на файлы с определёнными расширениями
                std::regex fileExtensionRegex(R"(\.(pdf|djvu|jpeg|jpg|doc|tiff|png|xls|css|zip|tar|7zip|bmp)$)");
                if (!std::regex_search(link, fileExtensionRegex)) {
                    Links.insert(link);
                }
            }
        }
    }

    // Рекурсивно обрабатываем дочерние элементы
    if (node->type == GUMBO_NODE_ELEMENT || node->type == GUMBO_NODE_DOCUMENT) {
        GumboVector* children = &node->v.element.children;
        for (unsigned int i = 0; i < children->length; ++i) {
            findLinks(static_cast<GumboNode*>(children->data[i]), SourceHost);
        }
    }
}
