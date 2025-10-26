#pragma once
#include <iostream>
#include "../Spider/ParcerINI.h"
#include "../Spider/database.h"
#include "logger.h"

// Класс-обёртка для подключения к БД — RAII гарантирует закрытие соединения
class DatabaseConnection {
public:
    explicit DatabaseConnection(ParcerINI& cfg)
    {
        db_.SetConnection(
            cfg.get_value<std::string>("DataBase.HostName"),
            cfg.get_value<std::string>("DataBase.DatabaseName"),
            cfg.get_value<std::string>("DataBase.UserName"),
            cfg.get_value<std::string>("DataBase.Password"),
            cfg.get_value<int>("DataBase.Port")
        );
    }

    ~DatabaseConnection() noexcept {
        try { db_.CloseConnection(); }
        catch (...) { LOG_ERR("Ошибка закрытия соединения с БД"); }
    }

    database& get() noexcept { return db_; }

private:
    database db_;
};
