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

static std::string detectCharset(const std::string& html) {
    std::regex charset_regex(R"(<meta\s+[^>]*charset\s*=\s*["']?([a-zA-Z0-9_-]+)["']?)", std::regex::icase);
    std::smatch match;
    if (std::regex_search(html, match, charset_regex)) {
        return match[1]; // найденная кодировка
    }
    return "UTF-8"; // по умолчанию
}

ParcerHTML::ParcerHTML(std::string HTML_strings, std::string SourceHost) {
    if (HTML_strings.empty()) {
        throw std::domain_error("HTML input is empty. Length: 0 bytes.");
    }

    // Определяем кодировку и конвертируем в UTF-8
    std::string charset = detectCharset(HTML_strings);
    try {
        HTML_strings = boost::locale::conv::to_utf<char>(HTML_strings, charset);
    }
    catch (const std::exception& e) {
        throw std::runtime_error("Charset conversion failed: " + std::string(e.what()));
    }

    // Устанавливаем глобальную локаль UTF-8
    std::locale::global(boost::locale::generator()("en_US.UTF-8"));

    // Парсим HTML с помощью Gumbo
    GumboOutput* output = gumbo_parse(HTML_strings.c_str());
    findLinks(output->root, SourceHost);
    gumbo_destroy_output(&kGumboDefaultOptions, output);

    // Удаляем теги и комментарии
    std::regex tagRegex(R"((<[^>]*>|<!--[^>]*-->))");
    Line = std::regex_replace(HTML_strings, tagRegex, " ");

    // Заменяем &nbsp; на пробелы
    std::regex pattern_nbsp(R"((&nbsp;\s*)+)");
    Line = std::regex_replace(Line, pattern_nbsp, " ");

    // Оставляем только буквы и цифры
    std::regex pattern_keep_alphanumeric(R"([^a-zA-Zа-яА-ЯёЁ0-9\s])");
    Line = std::regex_replace(Line, pattern_keep_alphanumeric, " ");

    // Переводим в нижний регистр с учётом локали
    boost::locale::generator gen;
    std::locale loc = gen("en_US.UTF-8");
    Line = boost::locale::to_lower(Line, loc);

    // Заменяем множественные пробелы на "_"
    std::regex SPACEpattern(R"(\s+)");
    Line = std::regex_replace(Line, SPACEpattern, "_");

    // Подсчитываем частоты слов
    size_t lineLength = Line.length();
    size_t cut_start_pos = 0;
    for (size_t iter = 0; iter <= lineLength; ++iter) {
        if (iter == lineLength || Line[iter] == '_') {
            std::string word = Line.substr(cut_start_pos, iter - cut_start_pos);
            if (word.length() >= 3 && word.length() <= 40) {
                Frequencies[word]++;
            }
            cut_start_pos = iter + 1;
        }
    }
}

bool ParcerHTML::isValidLink(const std::string& link) {
    std::regex fileExtensionRegex(R"(\.(pdf|djvu|jpeg|jpg|doc|tiff|png|xls|css|zip|tar|7zip|bmp|gif|svg|ico)$)", std::regex_constants::icase);
    return !std::regex_search(link, fileExtensionRegex);
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
        GumboAttribute* href = gumbo_get_attribute(&node->v.element.attributes, "href");
        if (href) {
            std::string link = href->value;

            if (!link.empty()) {
                // Удаляем фрагмент (#...)
                size_t pos = link.find('#');
                if (pos != std::string::npos)
                    link = link.substr(0, pos);
                if (link.empty()) return;

                // Проверяем абсолютную ссылку
                const std::string http_pref = "http://";
                const std::string https_pref = "https://";
                bool isHTTP = (link.compare(0, http_pref.length(), http_pref) == 0);
                bool isHTTPS = (link.compare(0, https_pref.length(), https_pref) == 0);

                // Если ссылка относительная — добавляем домен
                if (!isHTTP && !isHTTPS) {
                    std::string base = SourceHost;
                    if (!base.empty() && base.back() == '/') base.pop_back();
                    if (!link.empty() && link.front() != '/') link = "/" + link;
                    link = base + link;
                }

                // Удаляем завершающий слеш
                if (link.length() > 1 && link.back() == '/') link.pop_back();

                // Пропускаем медиафайлы и стили
                if (isValidLink(link)) {
                    Links.insert(link);
                }
            }
        }
    }

    // Рекурсивный обход
    if (node->type == GUMBO_NODE_ELEMENT || node->type == GUMBO_NODE_DOCUMENT) {
        GumboVector* children = &node->v.element.children;
        for (unsigned int i = 0; i < children->length; ++i) {
            findLinks(static_cast<GumboNode*>(children->data[i]), SourceHost);
        }
    }
}
