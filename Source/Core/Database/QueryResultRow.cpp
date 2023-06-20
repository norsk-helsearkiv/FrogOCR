#include "QueryResultRow.hpp"
#include "Core/Log.hpp"

namespace frog::database {

int QueryResultRow::integer(std::string_view column) const {
    const auto* value = raw(column);
    if (!value) {
        log::warning("Failed to get %cyan{}", column);
        return 0;
    }
	return from_string<int>(value).value_or(0);
}

long long QueryResultRow::long_integer(std::string_view column) const {
    const auto* value = raw(column);
    if (!value) {
        log::warning("Failed to get %cyan{}", column);
        return 0;
    }
	return from_string<long long>((value)).value_or(0);
}

float QueryResultRow::real(std::string_view column) const {
    const auto* value = raw(column);
    if (!value) {
        log::warning("Failed to get %cyan{}", column);
        return 0.0f;
    }
	return from_string<float>(value).value_or(0.0f);
}

bool QueryResultRow::boolean(std::string_view column) const {
	if (const auto* value = raw(column)) {
		return (*value == 't') || (*value == 'T') || (*value == '1');
	} else {
		return false;
	}
}

std::string QueryResultRow::text(std::string_view column) const {
	return PQgetvalue(result, row, column_index(column));
}

std::string_view QueryResultRow::text_view(std::string_view column_name) const {
	const auto column = column_index(column_name);
	return { PQgetvalue(result, row, column), static_cast<std::size_t>(PQgetlength(result, row, column)) };
}

std::optional<int> QueryResultRow::maybe_integer(std::string_view column) const {
	return is_null(column) ? std::optional<int>{} : integer(column);
}

std::optional<float> QueryResultRow::maybe_real(std::string_view column) const {
	return is_null(column) ? std::optional<float>{} : real(column);
}

std::optional<std::string> QueryResultRow::maybe_text(std::string_view column) const {
	return is_null(column) ? std::optional<std::string>{} : text(column);
}

char* QueryResultRow::raw(std::string_view column) const {
	return PQgetvalue(result, row, column_index(column));
}

int QueryResultRow::length(std::string_view column) const {
	return PQgetlength(result, row, column_index(column));
}

int QueryResultRow::column_index(std::string_view column) const {
	return PQfnumber(result, column.data());
}

bool QueryResultRow::exists(std::string_view column) const {
	return column_index(column) != -1;
}

bool QueryResultRow::is_null(std::string_view column) const {
	return PQgetisnull(result, row, column_index(column));
}

}
