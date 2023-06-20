#include "Connection.hpp"
#include "Core/Log.hpp"

namespace frog::database {

Connection::Connection(std::string_view host, int port, std::string_view database_name, std::string_view user, std::string_view password) {
    const auto parameters = fmt::format("host={} port={} dbname={} user={} password={}", host, port, database_name, user, password);
    connection = PQconnectdb(parameters.c_str());
    if (has_error()) {
        log::error("Failed to connect: {}", status_message());
    }
}

Connection::~Connection() {
    PQfinish(connection);
}

QueryResult Connection::execute(std::string_view query) const {
    return { PQexec(connection, query.data()) };
}

QueryResult Connection::execute(std::string_view query, const std::initializer_list<std::string_view>& params) const {
    if (params.size() > 65000) {
        log::error("Too many parameters: {}", params.size());
        return { nullptr };
    }
    const auto count = static_cast<int>(params.size());
    Oid* types{ nullptr }; // deduced by backend
    const char* values[16]{};
    for (std::size_t i{ 0 }; i < count; i++) {
        values[i] = (params.begin() + i)->data();
    }
    int* lengths{ nullptr };
    int* formats{ nullptr };
    int result_format{ 0 }; // text result
    return { PQexecParams(connection, query.data(), count, types, values, lengths, formats, result_format) };
}

QueryResult Connection::execute(std::string_view query, const std::vector<std::string>& params) const {
    if (params.size() > 65000) {
        log::error("Too many parameters: {}", params.size());
        return { nullptr };
    }
    const auto count = static_cast<int>(params.size());
    Oid* types{ nullptr }; // deduced by backend
    const char** values = new const char* [params.size()];
    for (std::size_t i{ 0 }; i < params.size(); i++) {
        values[i] = params[i].data();
    }
    int* lengths{ nullptr };
    int* formats{ nullptr };
    int result_format{ 0 }; // text result
    QueryResult result{ PQexecParams(connection, query.data(), count, types, values, lengths, formats, result_format) };
    delete[] values;
    return result;
}

bool Connection::has_error() const {
    return PQstatus(connection) != CONNECTION_OK;
}

std::string Connection::status_message() const {
    return PQerrorMessage(connection);
}

}
