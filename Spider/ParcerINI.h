#pragma once

#include <string>
#include <fstream>
#include <map>

class ParcerINI {
private:
	std::map<std::string, std::string> Sections;  // Хранит пары "Section.var" -> "value"

	// Ищет название секции в строке вида [Section]
	std::string section_finder(std::string str_for_find);

	// Ищет имя переменной в строке вида var = value
	std::string var_finder(std::string str_for_find);

	// Ищет значение переменной в строке вида var = value
	std::string var_value_finder(std::string str_for_find);

public:
	// Конструктор: читает ini-файл и сохраняет пары "Section.var" -> "value"
	ParcerINI(std::string file_path);

	// Получение значения переменной по ключу "Section.var" с нужным типом (int, double, string)
	template<typename T>
	T get_value(const std::string section_var);
};
