#include <iostream>
#include <tuple>
#include <pqxx/pqxx>
#include <windows.h>
#include <exception>
#include <string.h>
#include <thread>
#include "ParcerINI.h"
#include "database.h"
#include "indexator.h"

#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <queue>


class safe_queue {
	// Потокобезопасная очередь для хранения задач в виде std::function
	std::queue<std::function<void()>> tasks; // Очередь задач
	std::mutex mutex;                         // Мьютекс для блокировки очереди
	std::condition_variable cv;               // Условная переменная для уведомления потоков

public:
	int getTaskSize() { return tasks.size(); } // Получаем количество задач в очереди
	std::mutex& getMutex() { return mutex; }  // Доступ к мьютексу
	std::condition_variable& getCond() { return cv; } // Доступ к условной переменной

	// Добавляем задачу в очередь
	void push(std::function<void()> task) {
		std::unique_lock<std::mutex> lock(mutex);
		tasks.push(task);
		cv.notify_one(); // Уведомляем поток, что появилась новая задача
	}

	// Извлекаем задачу из очереди (ожидание, если очередь пуста)
	std::function<void()> pop() {
		std::unique_lock<std::mutex> lock(mutex);
		cv.wait(lock, [this] { return !tasks.empty(); });
		auto task = tasks.front();
		tasks.pop();
		return task;
	}
};

class thread_pool {
	safe_queue task_queue;        // Очередь задач для пула
	std::vector<std::thread> threads; // Потоки в пуле
	bool stop;                     // Флаг остановки пула
public:
	thread_pool(size_t num_threads) : stop(false) {
		// Запускаем num_threads потоков, которые будут выполнять задачи из очереди
		for (size_t i = 0; i < num_threads; ++i) {
			threads.emplace_back([this]() {
				while (task_queue.getTaskSize() > 0) { // Пока есть задачи
					auto task = task_queue.pop();      // Извлекаем задачу
					task();                             // Выполняем задачу
				}
				});
		}
	}

	~thread_pool() {
		// Дожидаемся завершения всех потоков
		for (auto& thread : threads) {
			if (thread.joinable()) thread.join();
		}
		stop = true;
	}

	// Добавляем задачу в очередь пула
	template <typename Func, typename... Args>
	void submit(Func&& func, Args&&... args) {
		task_queue.push(std::bind(std::forward<Func>(func), std::forward<Args>(args)...));
	}

	// Альтернативный метод для выполнения задач вручную
	void work() {
		while (task_queue.getTaskSize() > 0) {
			auto task = task_queue.pop();
			task();
		}
	}
};

// Рекурсивный многопоточный индексатор
void recursiveMultiTreadIndexator(database& DB, int Depth, std::set<std::string> inLinkSet) {
	if (Depth == 0) return; // Базовый случай рекурсии

	int threads_num = std::thread::hardware_concurrency(); // Определяем число потоков
	std::cout << "\nNumber of threads: " << threads_num << std::endl;
	std::cout << "\n\t Current recursion level: " << Depth << std::endl;
	std::cout << "\n\t Number of new links of current level: " << inLinkSet.size() << std::endl;

	std::mutex vectorMutex; // Мьютекс для безопасной записи результатов
	thread_pool pool(threads_num); // Создаём пул потоков
	std::vector<std::set<std::string>> outLinksVector; // Вектор для хранения ссылок из потоков
	int decrementLinks = inLinkSet.size(); // Счётчик оставшихся ссылок

	// Запускаем поток для добавления задач в пул
	std::thread T1([&pool, &DB, &inLinkSet, Depth, &outLinksVector, &vectorMutex, &decrementLinks]() mutable {
		for (const auto& newLink : inLinkSet) {
			pool.submit([&DB, newLink, Depth, &outLinksVector, &vectorMutex, &decrementLinks] {
				std::cout << "\nRecursion = " << Depth << " -> Left links: " << decrementLinks << " -> Task submitted for: " << newLink << std::endl;

				std::set<std::string> result = indexator(DB, newLink); // Индексируем страницу

				// Добавляем ссылки в общий вектор (с защитой мьютексом)
				std::lock_guard<std::mutex> lock(vectorMutex);
				outLinksVector.push_back(result);
				--decrementLinks;
				});
		}
		});
	T1.join(); // Ждём, пока все задачи будут поставлены

	// Поток для выполнения задач пула
	std::thread T2([&pool] { pool.work(); });
	T2.join(); // Ждём завершения выполнения всех задач

	// Объединяем результаты в один set
	std::set<std::string> outLinksSet;
	for (const auto& vectorIter : outLinksVector) {
		for (const auto& setIter : vectorIter) {
			outLinksSet.insert(setIter);
		}
	}

	// Рекурсивно индексируем следующий уровень глубины
	recursiveMultiTreadIndexator(DB, Depth - 1, outLinksSet);
}

// Основные настройки из конфигурации
std::string DataBaseHostName;
std::string DataBaseName;
std::string DataBaseUserName;
std::string DataBasePassword;
int DataBasePort;

std::string SpiderStarPageURL;
int SpiderDepth;

std::string FinderAddress;
int FinderPort;

int main() {
	SetConsoleCP(65001);       // UTF-8 для ввода
	SetConsoleOutputCP(65001); // UTF-8 для вывода

	try {
		std::string filePath = "C:\\Searcher\\Spider\\configuration.ini"; // Путь к INI
		ParcerINI parser = ParcerINI::ParcerINI(filePath);

		// Считываем настройки из файла
		DataBaseHostName = parser.get_value<std::string>("DataBase.HostName");
		DataBaseName = parser.get_value<std::string>("DataBase.DatabaseName");
		DataBaseUserName = parser.get_value<std::string>("DataBase.UserName");
		DataBasePassword = parser.get_value<std::string>("DataBase.Password");
		DataBasePort = parser.get_value<int>("DataBase.Port");

		SpiderStarPageURL = parser.get_value<std::string>("Spider.StartPageURL");
		SpiderDepth = parser.get_value<int>("Spider.Depth");

		FinderAddress = parser.get_value<std::string>("Finder.Address");
		FinderPort = parser.get_value<int>("Finder.Port");
	}
	catch (const std::exception& ex) {
		std::cout << "\nError reading configuration: " << ex.what();
	}

	database DB;

	try {
		DB.SetConnection(DataBaseHostName, DataBaseName, DataBaseUserName, DataBasePassword, DataBasePort);
		DB.table_delete(); // Удаляем старые таблицы
		DB.table_create(); // Создаём новые таблицы
	}
	catch (const std::exception& ex) {
		std::cout << "Error creating database tables: " << ex.what() << std::endl;
	}

	// Индексируем первую страницу
	std::set<std::string> inLinkSet = indexator(DB, SpiderStarPageURL);
	SpiderDepth--;

	// Запускаем рекурсивный многопоточный индексатор
	recursiveMultiTreadIndexator(DB, SpiderDepth, inLinkSet);

	DB.CloseConnection(); // Закрываем соединение с базой
}
