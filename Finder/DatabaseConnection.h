#pragma once
#include <iostream>
#include "../Spider/ParcerINI.h"
#include "../Spider/database.h"
#include "logger.h"

// �����-������ ��� ����������� � �� � RAII ����������� �������� ����������
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
        catch (...) { LOG_ERR("������ �������� ���������� � ��"); }
    }

    database& get() noexcept { return db_; }

private:
    database db_;
};
