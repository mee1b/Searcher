#pragma once
#include "session.h"

// Сервер — RAII: открывает сокет, закрывается при разрушении
class HttpServer {
    net::io_context ioc_;
    tcp::acceptor acceptor_;
    database& db_;
public:
    HttpServer(const std::string& addr, int port, database& db)
        : ioc_(1), acceptor_(ioc_, tcp::endpoint(net::ip::make_address(addr), port)), db_(db)
    {
    }

    void run() {
        do_accept();
        ioc_.run();
    }

private:
    void do_accept() {
        acceptor_.async_accept(
            [this](beast::error_code ec, tcp::socket socket) {
                if (!ec) std::make_shared<Session>(std::move(socket), db_)->run();
                do_accept();
            });
    }
};
