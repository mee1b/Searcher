#include "ParcerINI.h"

// Ищет название секции в строке вида [Section]
std::string ParcerINI::section_finder(std::string str_for_find) {
	int b_pos{ 0 }, e_pos{ 0 };
	b_pos = str_for_find.find('['); // ищем начало секции
	int com_pos = str_for_find.find(';'); // ищем комментарий
	if (b_pos < 0) { return ""; } // если нет '[', возвращаем пусто
	if (com_pos == 0) return "";   // если строка начинается с ';', игнорируем
	e_pos = str_for_find.find(']'); // ищем конец секции
	if (e_pos < 0) { return""; }
	return str_for_find.substr(b_pos + 1, e_pos - b_pos - 1); // возвращаем название секции
}

// Ищет имя переменной в строке вида var = value
std::string ParcerINI::var_finder(std::string str_for_find) {
	int com_pos = str_for_find.find(';');
	if (com_pos == 0) return ""; // игнорируем комментарии
	int equal_pos = str_for_find.find('=');
	if (equal_pos < 0) { return""; } // если нет '=', возвращаем пусто

	int b_pos{ 0 };
	std::string::iterator IT = str_for_find.begin();
	while ((*IT == ' ') || (*IT == '    ')) { IT++; b_pos++; } // пропускаем пробелы

	int symbol_num{ 0 };
	while ((*IT != ' ') && (*IT != '    ') && (*IT != '=')) { // читаем до '='
		IT++;
		symbol_num++;
		if (*IT == ';') return ""; // если встретили комментарий, возвращаем пусто
	}
	return str_for_find.substr(b_pos, symbol_num); // возвращаем имя переменной
}

// Ищет значение переменной в строке вида var = value
std::string ParcerINI::var_value_finder(std::string str_for_find) {
	int com_pos = str_for_find.find(';');
	if (com_pos == 0) return ""; // игнорируем комментарии
	int equal_pos = str_for_find.find('=');
	if (equal_pos < 0) { return ""; } // если нет '=', возвращаем пусто
	if (equal_pos == str_for_find.length() - 1) { return ""; } // если '=' в конце, пусто

	int b_pos = equal_pos + 1;
	std::string::iterator IT = str_for_find.begin() + b_pos;
	while ((*IT == ' ') || (*IT == '    ')) { IT++; b_pos++; } // пропускаем пробелы

	int symbol_num{ 1 };
	while (IT < str_for_find.end() - 1) {
		if ((*IT == ' ') || (*IT == '    ') || (*IT == ';')) { symbol_num--; break; } // конец значения
		IT++;
		symbol_num++;
	}
	return str_for_find.substr(b_pos, symbol_num); // возвращаем значение
}

// Конструктор: читает ini-файл и сохраняет пары section.var -> value
ParcerINI::ParcerINI(std::string file_path) {
	std::ifstream fin(file_path);
	if (!fin.is_open()) { throw std::invalid_argument("file is not exists"); }

	std::string line;
	std::string section = "";
	std::string var = "";
	std::string var_value = "";
	int line_number{ 0 };

	while (std::getline(fin, line)) {
		std::string tmp_str = "";
		tmp_str = section_finder(line); // ищем секцию
		if (tmp_str != "") { section = tmp_str; }

		tmp_str = var_finder(line); // ищем имя переменной
		if (tmp_str != "") { var = tmp_str; }

		tmp_str = var_value_finder(line); // ищем значение
		if ((tmp_str != "") && (var != "") && (section != "")) {
			var_value = tmp_str;
			Sections[section + "." + var] = var_value; // сохраняем в map
		}
		line_number++;
	}

	// Проверка на ошибки в файле
	if (section == "") { throw std::domain_error("no sections in file: " + file_path); }
	if (var == "" || var_value == "") { throw std::domain_error("no variables defined in file: " + file_path); }

	fin.close();
}

// Специализация шаблона для int
template<>
int ParcerINI::get_value(const std::string section_var) {
	auto answer = Sections[section_var];
	if (answer == "") { throw std::invalid_argument("no such pair section + var:" + section_var); }
	return std::stoi(Sections[section_var]);
}

// Специализация шаблона для double
template<>
double ParcerINI::get_value(const std::string section_var) {
	auto answer = Sections[section_var];
	if (answer == "") { throw std::invalid_argument("no such pair section + var:" + section_var); }
	return std::stod(answer);
}

// Специализация шаблона для string
template<>
std::string ParcerINI::get_value(const std::string section_var) {
	auto answer = Sections[section_var];
	if (answer == "") { throw std::invalid_argument("no such pair section+var: " + section_var); }
	return Sections[section_var];
}
