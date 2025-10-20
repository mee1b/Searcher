#pragma once

#include <string>
#include <fstream>
#include <map>

class ParcerINI {
private:
	std::map<std::string, std::string> Sections;  // ������ ���� "Section.var" -> "value"

	// ���� �������� ������ � ������ ���� [Section]
	std::string section_finder(std::string str_for_find);

	// ���� ��� ���������� � ������ ���� var = value
	std::string var_finder(std::string str_for_find);

	// ���� �������� ���������� � ������ ���� var = value
	std::string var_value_finder(std::string str_for_find);

public:
	// �����������: ������ ini-���� � ��������� ���� "Section.var" -> "value"
	ParcerINI(std::string file_path);

	// ��������� �������� ���������� �� ����� "Section.var" � ������ ����� (int, double, string)
	template<typename T>
	T get_value(const std::string section_var);
};
