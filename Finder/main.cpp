#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/strand.hpp>
#include <boost/config.hpp>
#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <boost/locale.hpp>
#include <boost/beast.hpp>

#include "../Spider/ParcerINI.h" // для чтения ini
#include "../Spider/database.h"   // подключение к базе
#include "finder.h"               // поисковик

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

// глобальная база данных
database DB;

// Определяем mime-type по расширению файла
beast::string_view mime_type(beast::string_view path)
{
	using beast::iequals;
	auto const ext = [&path]
		{
			auto const pos = path.rfind(".");
			if (pos == beast::string_view::npos)
				return beast::string_view{};
			return path.substr(pos);
		}();
	if (iequals(ext, ".htm"))  return "text/html";
	if (iequals(ext, ".html")) return "text/html";
	if (iequals(ext, ".php"))  return "text/html";
	if (iequals(ext, ".css"))  return "text/css";
	if (iequals(ext, ".txt"))  return "text/plain";
	if (iequals(ext, ".js"))   return "application/javascript";
	if (iequals(ext, ".json")) return "application/json";
	if (iequals(ext, ".xml"))  return "application/xml";
	if (iequals(ext, ".swf"))  return "application/x-shockwave-flash";
	if (iequals(ext, ".flv"))  return "video/x-flv";
	if (iequals(ext, ".png"))  return "image/png";
	if (iequals(ext, ".jpe"))  return "image/jpeg";
	if (iequals(ext, ".jpeg")) return "image/jpeg";
	if (iequals(ext, ".jpg"))  return "image/jpeg";
	if (iequals(ext, ".gif"))  return "image/gif";
	if (iequals(ext, ".bmp"))  return "image/bmp";
	if (iequals(ext, ".ico"))  return "image/vnd.microsoft.icon";
	if (iequals(ext, ".tiff")) return "image/tiff";
	if (iequals(ext, ".tif"))  return "image/tiff";
	if (iequals(ext, ".svg"))  return "image/svg+xml";
	if (iequals(ext, ".svgz")) return "image/svg+xml";
	return "application/text";
}

// Собираем полный путь к файлу
std::string path_cat(beast::string_view base, beast::string_view path)
{
	if (base.empty())
		return std::string(path);
	std::string result(base);
#ifdef BOOST_MSVC
	char constexpr path_separator = '\\';
	if (result.back() == path_separator)
		result.resize(result.size() - 1);
	result.append(path.data(), path.size());
	for (auto& c : result)
		if (c == '/')
			c = path_separator;
#else
	char constexpr path_separator = '/';
	if (result.back() == path_separator)
		result.resize(result.size() - 1);
	result.append(path.data(), path.size());
#endif
	return result;
}

// декодируем URL (замена %xx на символ)
std::string url_decode(const std::string& in) {
	std::string out;
	boost::beast::string_view sv(in);
	out.reserve(sv.size());

	for (std::size_t i = 0; i < sv.size(); ++i) {
		if (sv[i] == '%' && i + 2 < sv.size() &&
			std::isxdigit(sv[i + 1]) && std::isxdigit(sv[i + 2])) {
			int hi = std::isdigit(sv[i + 1]) ? sv[i + 1] - '0' : std::tolower(sv[i + 1]) - 'a' + 10;
			int lo = std::isdigit(sv[i + 2]) ? sv[i + 2] - '0' : std::tolower(sv[i + 2]) - 'a' + 10;
			out.push_back((hi << 4) | lo);
			i += 2;
		}
		else if (sv[i] == '+') {
			out.push_back(' ');
		}
		else {
			out.push_back(sv[i]);
		}
	}
	return out;
}

// Главная функция обработки запроса
template <class Body, class Allocator>
http::message_generator handle_request(
	beast::string_view doc_root,
	http::request<Body, http::basic_fields<Allocator>>&& req)
{
	// возвращаем ошибку
	auto const bad_request = [&req](beast::string_view why) {
		http::response<http::string_body> res{ http::status::bad_request, req.version() };
		res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
		res.set(http::field::content_type, "text/html");
		res.keep_alive(req.keep_alive());
		res.body() = std::string(why);
		res.prepare_payload();
		return res;
		};

	// только GET и POST
	if (req.method() != http::verb::get &&
		req.method() != http::verb::post)
		return bad_request("Unknown HTTP-method");

	// --- Главная страница ---
	if (req.method() == http::verb::get) {
		std::ostringstream body;
		body << "<!DOCTYPE html>\n"
			<< "<html lang=\"ru\">\n"
			<< "<head>\n"
			<< "  <meta charset=\"utf-8\">\n"
			<< "  <title>Поисковик</title>\n"
			<< "  <style>/* стили */</style>\n"
			<< "</head>\n"
			<< "<body>\n"
			<< "  <div class=\"container\">\n"
			<< "    <h1>Поисковик</h1>\n"
			<< "    <form action=\"/search\" method=\"POST\">\n"
			<< "      <input type=\"text\" name=\"query\" placeholder=\"Введите запрос\">\n"
			<< "      <input type=\"submit\" value=\"Найти\">\n"
			<< "    </form>\n"
			<< "  </div>\n"
			<< "</body>\n"
			<< "</html>\n";

		http::response<http::string_body> res{
			std::piecewise_construct,
			std::make_tuple(std::move(body.str())),
			std::make_tuple(http::status::ok, req.version()) };

		res.set(http::field::content_type, "text/html; charset=utf-8");
		res.content_length(res.body().size());
		res.keep_alive(req.keep_alive());
		return res;
	}

	// --- POST /search ---
	if (req.method() == http::verb::post && req.target() == "/search") {
		std::string body = req.body();
		std::string queryPrefix = "query=";
		std::string queryData = (body.substr(0, queryPrefix.size()) == queryPrefix)
			? url_decode(body.substr(queryPrefix.size()))
			: url_decode(body);

		std::vector<std::string> searchResults = finder(queryData, DB);
		std::ostringstream responseBody;

		responseBody << "<html>Результаты по запросу: " << queryData << "<br>";

		if (searchResults.empty()) {
			responseBody << "Ничего не найдено";
		}
		else {
			unsigned int count = 0;
			for (const auto& url : searchResults) {
				responseBody << "<a href=\"" << url << "\" target=\"_blank\">" << url << "</a><br>";
				if (++count >= 20) break;
			}
		}

		responseBody << "</html>";

		http::response<http::string_body> res{
			std::piecewise_construct,
			std::make_tuple(std::move(responseBody.str())),
			std::make_tuple(http::status::ok, req.version()) };

		res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
		res.set(http::field::content_type, "text/html; charset=utf-8");
		res.content_length(res.body().size());
		res.keep_alive(req.keep_alive());
		return res;
	}

	return bad_request("Что-то пошло не так");
}

//------------------------------------------------------------------------------

// Показываем ошибку
void fail(beast::error_code ec, char const* what)
{
	std::cerr << what << ": " << ec.message() << "\n";
}

// --- сессия с клиентом ---
class session : public std::enable_shared_from_this<session>
{
	beast::tcp_stream stream_;
	beast::flat_buffer buffer_;
	std::shared_ptr<std::string const> doc_root_;
	http::request<http::string_body> req_;

public:
	session(tcp::socket&& socket, std::shared_ptr<std::string const> const& doc_root)
		: stream_(std::move(socket)), doc_root_(doc_root) {
	}

	void run() {
		net::dispatch(stream_.get_executor(),
			beast::bind_front_handler(&session::do_read, shared_from_this()));
	}

	void do_read() {
		req_ = {};
		stream_.expires_after(std::chrono::seconds(30));
		http::async_read(stream_, buffer_, req_,
			beast::bind_front_handler(&session::on_read, shared_from_this()));
	}

	void on_read(beast::error_code ec, std::size_t bytes_transferred) {
		boost::ignore_unused(bytes_transferred);
		if (ec == http::error::end_of_stream)
			return do_close();
		if (ec)
			return fail(ec, "read");
		send_response(handle_request(*doc_root_, std::move(req_)));
	}

	void send_response(http::message_generator&& msg) {
		bool keep_alive = msg.keep_alive();
		beast::async_write(stream_, std::move(msg),
			beast::bind_front_handler(&session::on_write, shared_from_this(), keep_alive));
	}

	void on_write(bool keep_alive, beast::error_code ec, std::size_t bytes_transferred) {
		boost::ignore_unused(bytes_transferred);
		if (ec) return fail(ec, "write");
		if (!keep_alive) return do_close();
		do_read();
	}

	void do_close() {
		beast::error_code ec;
		stream_.socket().shutdown(tcp::socket::shutdown_send, ec);
	}
};

//------------------------------------------------------------------------------

// Слушатель для новых соединений
class listener : public std::enable_shared_from_this<listener>
{
	net::io_context& ioc_;
	tcp::acceptor acceptor_;
	std::shared_ptr<std::string const> doc_root_;

public:
	listener(net::io_context& ioc, tcp::endpoint endpoint,
		std::shared_ptr<std::string const> const& doc_root)
		: ioc_(ioc), acceptor_(net::make_strand(ioc)), doc_root_(doc_root)
	{
		beast::error_code ec;
		acceptor_.open(endpoint.protocol(), ec);
		if (ec) { fail(ec, "open"); return; }
		acceptor_.set_option(net::socket_base::reuse_address(true), ec);
		if (ec) { fail(ec, "set_option"); return; }
		acceptor_.bind(endpoint, ec);
		if (ec) { fail(ec, "bind"); return; }
		acceptor_.listen(net::socket_base::max_listen_connections, ec);
		if (ec) { fail(ec, "listen"); return; }
	}

	void run() { do_accept(); }

private:
	void do_accept() {
		acceptor_.async_accept(net::make_strand(ioc_),
			beast::bind_front_handler(&listener::on_accept, shared_from_this()));
	}

	void on_accept(beast::error_code ec, tcp::socket socket) {
		if (ec) { fail(ec, "accept"); return; }
		else std::make_shared<session>(std::move(socket), doc_root_)->run();
		do_accept();
	}
};

// конфиг базы
std::string DataBaseHostName;
std::string DataBaseName;
std::string DataBaseUserName;
std::string DataBasePassword;
int DataBasePort;

// адрес сервера поиска
std::string FinderAddress;
int FinderPort;

int main(int argc, char* argv[])
{
	SetConsoleCP(65001);
	SetConsoleOutputCP(65001); //UTF-8

	try {
		// читаем ini
		std::string filePath = "C:\\Searcher\\Spider\\configuration.ini";
		ParcerINI parser = ParcerINI(filePath);

		DataBaseHostName = parser.get_value<std::string>("DataBase.HostName");
		DataBaseName = parser.get_value<std::string>("DataBase.DatabaseName");
		DataBaseUserName = parser.get_value<std::string>("DataBase.UserName");
		DataBasePassword = parser.get_value<std::string>("DataBase.Password");
		DataBasePort = parser.get_value<int>("DataBase.Port");

		FinderAddress = parser.get_value<std::string>("Finder.Address");
		FinderPort = parser.get_value<int>("Finder.Port");

		// пробуем подключиться к базе
		try { DB.SetConnection(DataBaseHostName, DataBaseName, DataBaseUserName, DataBasePassword, DataBasePort); }
		catch (const std::exception& ex) { std::cout << "Try to create tables in databse\n" << ex.what(); }

		auto const address = net::ip::make_address(FinderAddress);
		auto const port = static_cast<unsigned short>(FinderPort);
		auto const doc_root = std::make_shared<std::string>(".");
		auto constexpr threads = std::max<int>(1, 1);

		net::io_context ioc{ threads };

		// запускаем слушатель
		std::make_shared<listener>(ioc, tcp::endpoint{ address, port }, doc_root)->run();

		std::vector<std::thread> v;
		v.reserve(threads - 1);
		for (auto i = threads - 1; i > 0; --i)
			v.emplace_back([&ioc] { ioc.run(); });
		ioc.run();

		return EXIT_SUCCESS;
	}
	catch (const std::exception& ex) {
		std::cout << "\n" << ex.what();
	}
}
