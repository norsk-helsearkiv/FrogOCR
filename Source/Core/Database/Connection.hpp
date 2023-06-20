#pragma once

#include "QueryResult.hpp"

namespace frog::database {

class Connection {
public:

    Connection(std::string_view host, int port, std::string_view database_name, std::string_view user, std::string_view password);
    Connection(const Connection&) = delete;
    Connection(Connection&&) = delete;

    ~Connection();

    Connection& operator=(const Connection&) = delete;
    Connection& operator=(Connection&&) = delete;

    QueryResult execute(std::string_view query) const;
    QueryResult execute(std::string_view query, const std::initializer_list<std::string_view>& params) const;
    QueryResult execute(std::string_view query, const std::vector<std::string>& params) const;

    bool has_error() const;
    std::string status_message() const;

private:

    PGconn* connection{ nullptr };

};

}
