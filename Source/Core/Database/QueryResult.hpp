#pragma once

#include "QueryResultRow.hpp"

namespace frog::database {

class QueryResult {
public:

    QueryResult(PGresult* result);
    QueryResult(const QueryResult&) = delete;
    QueryResult(QueryResult&& that) noexcept;

    ~QueryResult();

    QueryResult& operator=(const QueryResult&) = delete;
    QueryResult& operator=(QueryResult&&) = delete;

    QueryResultRow row(int index) const;
    int count() const;
    bool has_error() const;
    std::string status_message() const;

    operator bool() const {
        return !has_error();
    }

private:

    PGresult* result{ nullptr };

};

}
