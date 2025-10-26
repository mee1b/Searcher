#pragma once

#include <chrono>    // ��� ������ � �������� std::chrono::system_clock
#include <fstream>   // ��� ������ � �������� ������� std::ofstream
#include <ctime>     // ��� ������ � struct tm � �������� localtime_s
#include <mutex>     // ��� std::mutex � ������������������
#include <iomanip>   // ��� std::put_time � �������������� ����/�������


// ������������ ������� �����������
enum class log_level { INFO, WARN, ERR };

// ������� ��� ��������� ���� ������������ ����� �������
inline const char* project_relative(const char* path)
{
#ifdef PROJECT_ROOT
	// PROJECT_ROOT �������� ����� CMake ��� ������ � �������� ����� �������
	std::string root = PROJECT_ROOT;
	std::string full = path;

	// ������������ ������ �� Windows: '/' -> '\'
	for (auto& c : root) { if (c == '/') { c = '\\'; } }
	for (auto& c : full) { if (c == '/') { c = '\\'; } }

	// ��������, ���������� �� ���� � ����� � ����� �������
	if (full.find(root) == 0)
	{
		size_t offset = root.size();
		// ���� ����� ����� ���� ����, ���������� ���
		if (full[offset] == '\\') { offset++; }
		// ���������� ��������� �� ��������� ����� ����� �������
		return path + offset;
	}
#endif
	// ���� PROJECT_ROOT �� ��������� ��� ���� �� ���������� � �����, ���������� ������ ����
	return path;
}

// ����� Logger ��������� singleton ������
class Logger
{
public:

	// ����� ��������� ������������� ���������� �������
	static Logger& get_instance()
	{
		static Logger log; // ������� ������������� ������������ �������
		return log;
	}

	// ��������� ������������ ������ �����������
	void set_log_level(const log_level& lvl)
	{
		std::lock_guard<std::mutex> lock(mtx_); // ���������������
		min_lvl_ = lvl;
	}

	// �������� ����� �����������
	void log_in_file(const std::string& msg, const log_level& lvl,
		const char* file, int line, const char* func)
	{
		// ���������� ��������� ���� ������������ ������
		if (static_cast<int>(min_lvl_) > static_cast<int>(lvl)) { return; }

		// ���������������� ������
		std::lock_guard<std::mutex> lock(mtx_);

		// ��������� �������� �������
		auto now = time::now();
		std::time_t time_now = std::chrono::system_clock::to_time_t(now);
		std::tm local_tm;

		// �������������� ������� � ���������
		localtime_s(&local_tm, &time_now);

		// ������ � ����: �����, �������, ���� � ������, �������, ���������
		log_file << std::put_time(&local_tm, "%d-%m-%Y %H:%M:%S") << ' '
			<< choice_level(lvl)
			<< " (" << file << ':' << line << ") "
			<< func << ' ' << msg << std::endl;
	}

private:

	// ����������� (���������, ��� singleton)
	Logger()
	{
		// �������� ����� ����� � ������ ����������
		log_file.open("log.txt", std::ios::app);
		if (!log_file.is_open()) { throw std::runtime_error("������ �������� ��� �����!"); }

		// ������ ������ ����� ������ � ������� �����/��������
		auto now = time::now();
		std::time_t time_now = std::chrono::system_clock::to_time_t(now);
		std::tm local_tm;

		localtime_s(&local_tm, &time_now);

		log_file << std::put_time(&local_tm, "%d-%m-%Y %H:%M:%S")
			<< " [������ ������]" << std::endl;
	}

	// ���������� ��������� ����
	~Logger()
	{
		if (log_file.is_open()) { log_file.close(); }
	}

	// �������������� ������ ����������� � ������ ��� ������
	const char* choice_level(const log_level& lvl)
	{
		switch (lvl)
		{
		case log_level::INFO: return "INFO";             // �������������� ���������
		case log_level::WARN: return "---WARNING---";    // ��������������
		case log_level::ERR:  return "!!!ERROR!!!";     // ������
		default: return "UNKNOWN";                      // �� ������ ������
		}
	}

	log_level min_lvl_ = log_level::INFO;  // ����������� ������� �� ���������
	std::mutex mtx_;                       // ������� ��� ���������������� ������
	std::ofstream log_file;                // ����� ��� ����� �����

	using time = std::chrono::system_clock; // ������� ���������� ��� ������ � system_clock
};

// ������� ��� �������� �����������, ������������� ����������� ����, ������ � �������
#define LOG_INFO(msg) Logger::get_instance().log_in_file(msg, log_level::INFO, project_relative(__FILE__), __LINE__, __FUNCTION__)
#define LOG_WARN(msg) Logger::get_instance().log_in_file(msg, log_level::WARN, project_relative(__FILE__), __LINE__, __FUNCTION__) 
#define LOG_ERR(msg)  Logger::get_instance().log_in_file(msg, log_level::ERR,  project_relative(__FILE__), __LINE__, __FUNCTION__) 
