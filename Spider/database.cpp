#include "database.h" 
#include <iostream> 
#include <pqxx/pqxx> 
#include <exception> 
#include <tuple> 
#include <windows.h> 
#include <mutex> 

void database::SetConnection(std::string DataBaseHostName,
	std::string DataBaseName,
	std::string DataBaseUserName,
	std::string DataBasePassword,
	int DataBasePort) 
{
	std::string str_connection = "";

	// Собираем строку для подключения к Postgres
	str_connection.append("host=" + DataBaseHostName + " ");
	str_connection.append("port=" + std::to_string(DataBasePort) + " ");
	str_connection.append("dbname=" + DataBaseName + " ");
	str_connection.append("user=" + DataBaseUserName + " ");
	str_connection.append("password=" + DataBasePassword);

	// Создаём подключение
	std::unique_ptr<pqxx::connection> c1 = std::make_unique<pqxx::connection>(str_connection);
	c = std::move(c1);
	(*c).set_client_encoding("UTF8"); // чтобы русские буквы нормально читались
}

database::database() {
	; // пустой конструктор
}

void database::table_create() {
	// создаём таблицы, если их нет
	pqxx::work tx{ *c };
	tx.exec(str_creation);
	tx.commit();
}

void database::table_delete() {
	// удаляем таблицы
	pqxx::work tx{ *c };
	tx.exec(str_delete);
	tx.commit();
}

void database::CloseConnection() {
	// просто фиксируем изменения и закрываем (mutex на всякий случай)
	std::lock_guard<std::mutex> lock(mtx);
	pqxx::work tx{ *c };
	tx.commit();
}

void database::word_add(const std::string newWord) {
	std::lock_guard<std::mutex> lock(mtx); // блокируем на время добавления
	try
	{
		pqxx::work tx{ *c };
		// добавляем слово, если его ещё нет
		std::string str_word_add = "INSERT INTO Words (word) VALUES ('" + tx.esc(newWord) + "') ON CONFLICT (word) DO NOTHING;";
		tx.exec(str_word_add);
		tx.commit();
	}
	catch (const std::exception& ex) {
		// если что-то пошло не так
		std::cout << "\n\t" << __FILE__ << ", line: " << __LINE__;
		std::cout << "\nFail to add new word: " << newWord;
		std::cout << "\n" << ex.what();
	}
}

void database::link_add(const std::string newLink) {
	std::lock_guard<std::mutex> lock(mtx); // блокируем на время добавления
	try
	{
		pqxx::work tx{ *c };
		// добавляем ссылку, если её ещё нет
		std::string str_link_add = "INSERT INTO Links (link) VALUES ('" + tx.esc(newLink) + "') ON CONFLICT (link) DO NOTHING;";
		tx.exec(str_link_add);
		tx.commit();
	}
	catch (const std::exception& ex) {
		std::cout << "\n\t" << __FILE__ << ", line: " << __LINE__;
		std::cout << "\nFail to add new link: " << newLink;
		std::cout << "\n" << ex.what();
	}
}

std::map <std::string, int> database::getWordId() {
	std::lock_guard<std::mutex> lock(mtx); // блокируем на время чтения
	std::map<std::string, int> wordIdMap;
	try
	{
		pqxx::work tx{ *c };
		std::string select_word_id_pair = ("SELECT id, word FROM Words");
		pqxx::result result = tx.exec(select_word_id_pair);
		// заполняем map слово -> id
		for (size_t row = 0; row < result.size(); ++row) {
			int id = result[row].at("id").as<int>();
			std::string word = result[row].at("word").as<std::string>();
			wordIdMap[word] = id;
		}
		return wordIdMap;
	}
	catch (const std::exception& ex) {
		std::cout << "\n\t" << __FILE__ << ", line: " << __LINE__;
		std::cout << "\nFail to get all words ID: ";
		std::cout << "\n" << ex.what();
	}
}

int database::getLinkId(const std::string& linkValue) {
	std::lock_guard<std::mutex> lock(mtx); // блокируем на время чтения
	try
	{
		pqxx::work tx{ *c };
		std::string select_link_id = "SELECT id FROM Links WHERE link = '" + tx.esc(linkValue) + "'";
		pqxx::result result = tx.exec(select_link_id);
		if (!result.empty()) {
			int id = result[0][0].as<int>();
			return id;
		}
		else {
			// если ссылки нет в базе
			return -1;
		}
	}
	catch (const std::exception& ex) {
		std::cout << "\n\t" << __FILE__ << ", line: " << __LINE__;
		std::cout << "\nFail to get link ID: " << linkValue;
		std::cout << "\n" << ex.what();
		return -1;
	}
}

void database::frequency_add(const int linkID, const int wordID, const int frequency) {
	std::lock_guard<std::mutex> lock(mtx); // блокируем на время записи
	try
	{
		pqxx::work tx{ *c };
		// добавляем частоту слова на ссылке, если ещё нет
		std::string insert_frequency = "INSERT INTO frequencies (links_id, words_id, frequency) VALUES ("
			+ tx.quote(linkID) + ", "
			+ tx.quote(wordID) + ", "
			+ tx.quote(frequency) + ") ON CONFLICT (links_id, words_id) DO NOTHING;";
		tx.exec(insert_frequency);
		tx.commit();
	}
	catch (const std::exception& ex) {
		std::cout << "\n\t" << __FILE__ << ", line: " << __LINE__;
		std::cout << "\nFail to frequency_add: " << linkID << ": " << wordID << ": " << frequency;
		std::cout << "\n" << ex.what();
	}
}

std::map<std::string, int> database::seachRequest(std::string word_to_search) {
	std::lock_guard<std::mutex> lock(mtx); // блокируем на время чтения
	std::map<std::string, int> linksAndFrequency;
	try
	{
		pqxx::work tx{ *c };
		// ищем ссылки и частоты для слова
		std::string search_request = "SELECT links.link, frequencies.frequency FROM links "
			"JOIN frequencies ON links.id = frequencies.links_id "
			"JOIN words ON words.id = frequencies.words_id "
			"WHERE words.word = " + tx.quote(word_to_search) +
			" ORDER BY frequencies.frequency DESC LIMIT 100;";
		pqxx::result result_set = tx.exec(search_request);
		for (const pqxx::row& row : result_set) {
			std::string link = row[0].as<std::string>();
			int frequency = row[1].as<int>();
			linksAndFrequency[link] = frequency; // сохраняем в map
		}
	}
	catch (const std::exception& ex) {
		std::cout << "\n\t" << __FILE__ << ", line: " << __LINE__;
		std::cout << "\nFail to word seach " << word_to_search;
		std::cout << "\n" << ex.what();
	}
	return linksAndFrequency;
}
