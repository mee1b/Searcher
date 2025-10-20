#include "HTTPClient.h"
#include "root_certificates.hpp"
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/locale.hpp>
#include <regex>
#include <cstdlib>
#include <iostream>
#include <string>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ssl = net::ssl;
using tcp = net::ip::tcp;

// Обработка редиректа на новый URL
void HTTPclient::handleRedirect(const std::string& newLink, const std::string& port, int version) {
	std::string newHost;
	std::string newTarget;
	std::string newPort;

	// Определяем протокол по префиксу
	const std::string http_pref = "http://";
	const std::string https_pref = "https://";
	bool isHTTPS = false;
	bool isHTTP = false;

	if (newLink.compare(0, https_pref.length(), https_pref) == 0) isHTTPS = true;
	else if (newLink.compare(0, http_pref.length(), http_pref) == 0) isHTTP = true;
	else { // если префикс отсутствует, определяем по порту
		newHost = newLink;
		isHTTPS = (port == "443");
		isHTTP = (port == "80");
	}

	// Извлекаем порт из URL, если указан
	size_t portDelimiter = newHost.find(':');
	if (portDelimiter != std::string::npos) {
		newPort = newHost.substr(portDelimiter + 1);
		newHost = newHost.substr(0, portDelimiter);
	}
	else {
		newPort = isHTTPS ? "443" : "80";
	}

	// Убираем префикс протокола
	std::regex pattern_https(https_pref);
	std::regex pattern_http(http_pref);
	newHost = isHTTPS ? std::regex_replace(newLink, pattern_https, "") : std::regex_replace(newLink, pattern_http, "");

	// Разделяем host и target
	size_t slashPos = newHost.find("/");
	if (slashPos != std::string::npos) {
		std::string temp_str = newHost;
		newHost = newHost.substr(0, slashPos);

		size_t questionMarkPos = temp_str.find("?");
		if (questionMarkPos != std::string::npos) temp_str = temp_str.substr(0, questionMarkPos);

		newTarget = temp_str.substr(slashPos);
	}
	else {
		newTarget = "/";
	}

	newPort = isHTTPS ? "443" : "80";

	// Выполнение GET-запроса по новому адресу
	performGetRequest(newHost, newPort, newTarget, version);
}

// Выполнение GET-запроса по host/port/target
void HTTPclient::performGetRequest(const std::string& host, const std::string& port,
	const std::string& target, int version_in) {

	int version = version_in; // версия HTTP
	std::string charset; // кодировка страницы
	std::stringstream response_stream; // буфер ответа

	net::io_context ioc;

	if (port == "80") { // обычный HTTP
		tcp::resolver resolver(ioc);
		beast::tcp_stream stream(ioc);

		auto const results = resolver.resolve(host, port); // резолв домена
		stream.connect(results); // соединяемся

		// Формируем HTTP GET запрос
		http::request<http::string_body> req{ http::verb::get, target, version };
		req.set(http::field::host, host);
		req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

		http::write(stream, req); // отправляем запрос

		beast::flat_buffer buffer; // буфер чтения
		http::response<http::dynamic_body> res;
		http::read(stream, buffer, res); // читаем ответ

		// Проверка редиректа
		if (res.result() == http::status::moved_permanently) {
			auto newLocation = res.find(http::field::location);
			if (newLocation != res.end()) {
				handleRedirect(std::string(newLocation->value()), port, version);
				return;
			}
		}

		// Определяем charset из заголовка Content-Type
		auto contentTypeHeader = res.find("Content-Type");
		std::string TypeHeaderStr = std::string(contentTypeHeader->value());
		std::regex charsetPattern(R"(charset=([^\s;]+))", std::regex::icase);
		std::smatch match;
		if (std::regex_search(TypeHeaderStr, match, charsetPattern) && match.size() > 1)
			charset = match[1];

		response_stream << res; // сохраняем ответ

		// Закрываем сокет
		beast::error_code ec;
		stream.socket().shutdown(tcp::socket::shutdown_both, ec);
		if (ec && ec != beast::errc::not_connected) throw beast::system_error{ ec };
	}
	else if (port == "443") { // HTTPS
		ssl::context ctx(ssl::context::tlsv12_client);
		load_root_certificates(ctx);
		ctx.set_verify_mode(ssl::verify_none);

		tcp::resolver resolver(ioc);
		beast::ssl_stream<beast::tcp_stream> stream(ioc, ctx);

		if (!SSL_set_tlsext_host_name(stream.native_handle(), host.c_str()))
			throw beast::system_error{ beast::error_code{static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()} };

		auto const results = resolver.resolve(host.c_str(), port.c_str());
		beast::get_lowest_layer(stream).connect(results); // соединяемся

		stream.handshake(ssl::stream_base::client); // SSL handshake

		// GET-запрос
		http::request<http::string_body> req{ http::verb::get, target, version };
		req.set(http::field::host, host);
		req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
		http::write(stream, req);

		beast::flat_buffer buffer;
		http::response<http::dynamic_body> res;
		http::read(stream, buffer, res);

		// Редирект
		if (res.result() == http::status::moved_permanently) {
			auto newLocation = res.find(http::field::location);
			if (newLocation != res.end()) {
				handleRedirect(std::string(newLocation->value()), port, version);
				return;
			}
		}

		// charset из Content-Type
		auto contentTypeHeader = res.find("Content-Type");
		std::string TypeHeaderStr = std::string(contentTypeHeader->value());
		std::regex charsetPattern(R"(charset=([^\s;]+))", std::regex::icase);
		std::smatch match;
		if (std::regex_search(TypeHeaderStr, match, charsetPattern) && match.size() > 1)
			charset = match[1];

		response_stream << res;

		// Graceful shutdown SSL
		beast::error_code ec;
		stream.shutdown(ec);
		if (ec == net::error::eof) ec = {};
		if (ec) throw beast::system_error{ ec };
	}
	else {
		throw std::domain_error("\nThis is not HTTP or HTTPS port: " + port + "\n");
	}

	// Собираем все строки страницы в одну
	std::string line;
	while (std::getline(response_stream, line)) {
		lines.append(" ");
		lines.append(line);
	}

	// Если charset не указан, ищем его в meta tag
	if (charset.empty()) {
		std::regex charsetPattern_(R"(charset\s*['"]?([^'">\s]+)['"]?\s*[>,;\s*])", std::regex::icase);
		std::smatch match_;
		if (std::regex_search(lines, match_, charsetPattern_)) charset = match_[1];
	}

	// Перекодировка в UTF-8
	const std::string UTF8{ "UTF-8" };
	if (!charset.empty() && charset != UTF8) lines = boost::locale::conv::between(lines, UTF8, charset);
}

// Получение уже загруженных данных страницы
std::string HTTPclient::getData() {
	if (!lines.empty()) return lines;
}
