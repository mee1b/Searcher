#include <boost/beast/http/message_generator.hpp>
#include <boost/beast/http/write.hpp>
#include <boost/beast/version.hpp>
#include <iostream>
#include <memory>

#include "DatabaseConnection.h"
#include "finder.h"
#include "httpserver.h"

int main() {
    SetConsoleCP(65001);
    SetConsoleOutputCP(65001);
    LOG_INFO("start!");

    try 
    {
        ParcerINI cfg("C:\\Searcher\\Spider\\configuration.ini");
        DatabaseConnection dbConn(cfg);

        std::string addr = cfg.get_value<std::string>("Finder.Address");
        int port = cfg.get_value<int>("Finder.Port");

        HttpServer server(addr, port, dbConn.get());
        server.run();
    }
    catch (const std::exception& ex) {
        std::string er = ex.what();
        LOG_ERR("Ошибка: " + er);
        return EXIT_FAILURE;
    }
}
