#include "QueryResult.hpp"
#include "Core/Log.hpp"

namespace frog::database {

QueryResult::QueryResult(PGresult* result) : result{ result } {
    if (has_error()) {
        log::warning("Query failed. Error: {}", status_message());
    }
}

QueryResult::QueryResult(QueryResult&& that) noexcept {
    result = that.result;
    that.result = nullptr;
}

QueryResult::~QueryResult() {
    PQclear(result);
}

QueryResultRow QueryResult::row(int index) const {
    return { result, index };
}

int QueryResult::count() const {
    return PQntuples(result);
}

bool QueryResult::has_error() const {
    switch (PQresultStatus(result)) {
        case PGRES_BAD_RESPONSE:
        case PGRES_FATAL_ERROR:
        case PGRES_NONFATAL_ERROR:
            return true;
        default:
            return false;
    }
}

std::string QueryResult::status_message() const {
    return PQresultVerboseErrorMessage(result, PQERRORS_VERBOSE, PQSHOW_CONTEXT_ALWAYS);
}

}
