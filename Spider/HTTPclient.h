#pragma once

#include <string>
#include <vector>

class HTTPclient {

public:
	// Выполнить HTTP GET запрос к host:port/target с указанной версией HTTP
	void performGetRequest(const std::string& host, const std::string& port,
		const std::string& target, int version_in);

	// Получить результат последнего запроса в виде одной строки HTML
	std::string getData();

private:
	std::string lines; // HTML страницы, собранный в одну строку
	// Обработать редирект на новый URL и выполнить повторный запрос
	void handleRedirect(const std::string& newLink, const std::string& port, int version);
};
