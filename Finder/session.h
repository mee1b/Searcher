#pragma once
#include "utils.h"

namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

// HTTP-сессия
class Session : public std::enable_shared_from_this<Session> {
    beast::tcp_stream stream_;
    beast::flat_buffer buffer_;
    http::request<http::string_body> req_;
    database& db_;

    std::optional<http::response<http::string_body>> res_; 
public:
    Session(tcp::socket&& socket, database& db)
        : stream_(std::move(socket)), db_(db) {
    }

    void run() { do_read(); }

private:
    void do_read() {
        req_ = {};
        http::async_read(stream_, buffer_, req_,
            [self = shared_from_this()](beast::error_code ec, std::size_t) {
                if (!ec)
                    self->on_request();
            });
    }

    void on_request() {
        res_.emplace(handle_request(std::move(req_), db_));

        http::async_write(stream_, *res_,
            [self = shared_from_this()](beast::error_code ec, std::size_t) {
                self->on_write(ec);
            });
    }

    void on_write(beast::error_code ec) {
        if (ec)
        {
            std::string er = ec.message();
            LOG_ERR("Ошибка записи: " + er);
        }

        beast::error_code ignore_ec;
        stream_.socket().shutdown(tcp::socket::shutdown_send, ignore_ec);
    }
};