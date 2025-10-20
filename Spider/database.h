#pragma once

#include <mutex>
#include <pqxx/pqxx>
#include <windows.h>
#include <set>

class database {
private:
	std::unique_ptr <pqxx::connection> c; // соединение с базой
	std::mutex mtx; // защищаем операции с базой (thread-safe)

	// SQL для создания таблиц, если их нет
	std::string str_creation = {
			"CREATE TABLE IF NOT EXISTS links ("
			"id SERIAL PRIMARY KEY, "
			"link TEXT UNIQUE NOT NULL); "
			"CREATE TABLE IF NOT EXISTS words ("
			"id SERIAL PRIMARY KEY, "
			"word VARCHAR(40) UNIQUE NOT NULL); "
			"CREATE TABLE IF NOT EXISTS frequencies ("
			"links_id INTEGER REFERENCES links(id), "
			"words_id INTEGER REFERENCES words(id), "
			"frequency INTEGER, "
			"CONSTRAINT pk PRIMARY KEY(links_id, words_id));"
	};

	// SQL для удаления таблиц
	std::string str_delete = {
			"DROP TABLE IF EXISTS frequencies; "
			"DROP TABLE IF EXISTS words; "
			"DROP TABLE IF EXISTS links;"
	};

public:
	database(); // конструктор

	// устанавливаем соединение с базой
	void SetConnection(std::string DataBaseHostName,
		std::string DataBaseName,
		std::string DataBaseUserName,
		std::string DataBasePassword,
		int DataBasePort);

	void table_create(); // создаём таблицы
	void table_delete(); // удаляем таблицы
	void CloseConnection(); // фиксируем изменения и закрываем соединение

	// запрещаем копирование объекта
	database(const database&) = delete;
	database& operator=(const database&) = delete;

	void word_add(const std::string newWord); // добавляем слово в базу
	void link_add(const std::string newLink); // добавляем ссылку в базу

	std::map < std::string, int> getWordId(); // возвращает map слово->id
	int getLinkId(const std::string& linkValue); // возвращает id ссылки

	void frequency_add(const int linkID, const int wordID, const int frequency); // добавляем частоту слова на ссылке
	std::map<std::string, int> seachRequest(std::string word_to_search); // ищем ссылки и частоты по слову
};
