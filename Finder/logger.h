#pragma once

#include <chrono>    // Для работы с временем std::chrono::system_clock
#include <fstream>   // Для работы с файловым потоком std::ofstream
#include <ctime>     // Для работы с struct tm и функцией localtime_s
#include <mutex>     // Для std::mutex и потокобезопасности
#include <iomanip>   // Для std::put_time и форматирования даты/времени


// Перечисление уровней логирования
enum class log_level { INFO, WARN, ERR };

// Функция для получения пути относительно корня проекта
inline const char* project_relative(const char* path)
{
#ifdef PROJECT_ROOT
	// PROJECT_ROOT задается через CMake как макрос с корневым путем проекта
	std::string root = PROJECT_ROOT;
	std::string full = path;

	// Нормализация слэшей на Windows: '/' -> '\'
	for (auto& c : root) { if (c == '/') { c = '\\'; } }
	for (auto& c : full) { if (c == '/') { c = '\\'; } }

	// Проверка, начинается ли путь к файлу с корня проекта
	if (full.find(root) == 0)
	{
		size_t offset = root.size();
		// Если после корня идет слэш, пропускаем его
		if (full[offset] == '\\') { offset++; }
		// Возвращаем указатель на подстроку после корня проекта
		return path + offset;
	}
#endif
	// Если PROJECT_ROOT не определен или путь не начинается с корня, возвращаем полный путь
	return path;
}

// Класс Logger реализует singleton логгер
class Logger
{
public:

	// Метод получения единственного экземпляра логгера
	static Logger& get_instance()
	{
		static Logger log; // Ленивая инициализация статического объекта
		return log;
	}

	// Установка минимального уровня логирования
	void set_log_level(const log_level& lvl)
	{
		std::lock_guard<std::mutex> lock(mtx_); // Потокобезопасно
		min_lvl_ = lvl;
	}

	// Основной метод логирования
	void log_in_file(const std::string& msg, const log_level& lvl,
		const char* file, int line, const char* func)
	{
		// Пропускаем сообщения ниже минимального уровня
		if (static_cast<int>(min_lvl_) > static_cast<int>(lvl)) { return; }

		// Потокобезопасная запись
		std::lock_guard<std::mutex> lock(mtx_);

		// Получение текущего времени
		auto now = time::now();
		std::time_t time_now = std::chrono::system_clock::to_time_t(now);
		std::tm local_tm;

		// Преобразование времени в локальное
		localtime_s(&local_tm, &time_now);

		// Запись в файл: время, уровень, файл и строка, функция, сообщение
		log_file << std::put_time(&local_tm, "%d-%m-%Y %H:%M:%S") << ' '
			<< choice_level(lvl)
			<< " (" << file << ':' << line << ") "
			<< func << ' ' << msg << std::endl;
	}

private:

	// Конструктор (приватный, для singleton)
	Logger()
	{
		// Открытие файла логов в режиме добавления
		log_file.open("log.txt", std::ios::app);
		if (!log_file.is_open()) { throw std::runtime_error("Ошибка открытия лог файла!"); }

		// Запись начала новой сессии с текущей датой/временем
		auto now = time::now();
		std::time_t time_now = std::chrono::system_clock::to_time_t(now);
		std::tm local_tm;

		localtime_s(&local_tm, &time_now);

		log_file << std::put_time(&local_tm, "%d-%m-%Y %H:%M:%S")
			<< " [НАЧАЛО СЕССИИ]" << std::endl;
	}

	// Деструктор закрывает файл
	~Logger()
	{
		if (log_file.is_open()) { log_file.close(); }
	}

	// Преобразование уровня логирования в строку для записи
	const char* choice_level(const log_level& lvl)
	{
		switch (lvl)
		{
		case log_level::INFO: return "INFO";             // Информационное сообщение
		case log_level::WARN: return "---WARNING---";    // Предупреждение
		case log_level::ERR:  return "!!!ERROR!!!";     // Ошибка
		default: return "UNKNOWN";                      // На всякий случай
		}
	}

	log_level min_lvl_ = log_level::INFO;  // Минимальный уровень по умолчанию
	std::mutex mtx_;                       // Мьютекс для потокобезопасной записи
	std::ofstream log_file;                // Поток для файла логов

	using time = std::chrono::system_clock; // Удобное сокращение для работы с system_clock
};

// Макросы для удобного логирования, автоматически подставляют файл, строку и функцию
#define LOG_INFO(msg) Logger::get_instance().log_in_file(msg, log_level::INFO, project_relative(__FILE__), __LINE__, __FUNCTION__)
#define LOG_WARN(msg) Logger::get_instance().log_in_file(msg, log_level::WARN, project_relative(__FILE__), __LINE__, __FUNCTION__) 
#define LOG_ERR(msg)  Logger::get_instance().log_in_file(msg, log_level::ERR,  project_relative(__FILE__), __LINE__, __FUNCTION__) 
