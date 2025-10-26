#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <cctype>
#include <algorithm>

#include <boost/beast/http.hpp>
#include <boost/beast/core.hpp>
#include <boost/asio.hpp>

#include "../Spider/database.h"
#include "finder.h"  

namespace beast = boost::beast;
namespace http = beast::http;

//------------------------------------------------------------------------------
// Утилита для декодирования URL
std::string url_decode(const std::string& in) {
    std::string out;
    for (size_t i = 0; i < in.size(); ++i) {
        if (in[i] == '%' && i + 2 < in.size()) {
            int hi = std::isdigit(in[i + 1]) ? in[i + 1] - '0' : std::tolower(in[i + 1]) - 'a' + 10;
            int lo = std::isdigit(in[i + 2]) ? in[i + 2] - '0' : std::tolower(in[i + 2]) - 'a' + 10;
            out.push_back((hi << 4) | lo);
            i += 2;
        }
        else if (in[i] == '+') out.push_back(' ');
        else out.push_back(in[i]);
    }
    return out;
}

//------------------------------------------------------------------------------
// Генерация HTML-ответа
std::string make_html_response(const std::string& query, const std::vector<std::string>& results) {
    std::ostringstream body;
    body << "<!DOCTYPE html>\n<html lang=\"ru\">\n<head>\n"
        << "<meta charset=\"utf-8\">\n"
        << "<title>Поисковик</title>\n"
        << "<style>\n"
        << "  body { font-family: 'Segoe UI', Tahoma, sans-serif; background: #f4f4f9; margin: 0; padding: 0; display: flex; flex-direction: column; align-items: center; }\n"
        << "  header { background-color: #4a76a8; color: white; padding: 2em; text-align: center; width: 100%; box-shadow: 0 2px 5px rgba(0,0,0,0.1); }\n"
        << "  h1 { margin: 0; font-size: 2em; }\n"
        << "  main { max-width: 800px; width: 90%; margin: 2em auto; padding: 2em; background: white; border-radius: 10px; box-shadow: 0 4px 10px rgba(0,0,0,0.1); }\n"
        << "  .result { margin: 1em 0; padding: 1em; border-bottom: 1px solid #eee; background: #fafafa; border-radius: 5px; }\n"
        << "  .result a { text-decoration: none; color: #4a76a8; font-weight: bold; }\n"
        << "  .result a:hover { text-decoration: underline; }\n"
        << "  form { margin-top: 2em; display: flex; gap: 0.5em; justify-content: center; }\n"
        << "  input[name='query'] { padding: 0.7em; flex: 1; border-radius: 5px; border: 1px solid #ccc; font-size: 1em; }\n"
        << "  input[type='submit'] { padding: 0.7em 1.5em; border: none; background-color: #4a76a8; color: white; border-radius: 5px; cursor: pointer; font-size: 1em; }\n"
        << "  input[type='submit']:hover { background-color: #3a5c82; }\n"
        << "  .search-info { margin-bottom: 1em; color: #555; }\n"
        << "</style>\n"
        << "</head>\n<body>\n"
        << "<header><h1>Поисковик</h1></header>\n"
        << "<main>\n";

    // Если есть запрос, показываем блок с информацией
    if (!query.empty()) {
        body << "<div class=\"search-info\">Результаты по запросу: <strong>" << query << "</strong></div>\n";
    }

    // Если есть результаты, показываем их
    if (!results.empty()) {
        for (const auto& url : results)
            body << "<div class=\"result\"><a href=\"" << url << "\" target=\"_blank\">" << url << "</a></div>\n";
    }
    else if (!query.empty()) {
        // Если запрос был, но результатов нет
        body << "<p>Ничего не найдено.</p>\n";
    }

    // Форма поиска всегда видна, с заполненным текстом запроса
    body << "<form action=\"/search\" method=\"POST\">\n"
        << "<input type=\"text\" name=\"query\" placeholder=\"Введите запрос\" value=\"" << query << "\">\n"
        << "<input type=\"submit\" value=\"Найти\">\n"
        << "</form>\n"
        << "</main>\n</body>\n</html>";

    return body.str();
}


//------------------------------------------------------------------------------
// Основной обработчик запросов
template <class Body, class Allocator>
http::response<http::string_body>
handle_request(http::request<Body, http::basic_fields<Allocator>>&& req, database& db)
{
    if (req.method() != http::verb::get && req.method() != http::verb::post)
        return { http::status::bad_request, req.version() };

    std::string query;
    std::vector<std::string> results;

    // Если POST-запрос на /search, получаем query и делаем поиск
    if (req.method() == http::verb::post && req.target() == "/search") {
        std::string body = req.body();
        std::string queryPrefix = "query=";
        query = (body.rfind(queryPrefix, 0) == 0)
            ? url_decode(body.substr(queryPrefix.size()))
            : url_decode(body);

        SearchEngine engine(db);
        results = engine.search(query);
    }

    // Генерируем HTML через make_html_response, query и results могут быть пустыми
    std::string html = make_html_response(query, results);

    http::response<http::string_body> res{ http::status::ok, req.version() };
    res.set(http::field::content_type, "text/html; charset=utf-8");
    res.body() = std::move(html);
    res.prepare_payload();
    return res;
}