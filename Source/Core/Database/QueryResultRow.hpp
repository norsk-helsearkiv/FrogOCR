#pragma once

#include <libpq/libpq-fe.h>

#include <string>
#include <functional>
#include <optional>

namespace frog::database {

class QueryResultRow {
public:

	QueryResultRow(PGresult* result, int row) : result{ result }, row{ row } {}

	int integer(std::string_view column) const;
	long long long_integer(std::string_view column) const;
	float real(std::string_view column) const;
	bool boolean(std::string_view column) const;
	std::string text(std::string_view column) const;
	std::string_view text_view(std::string_view column) const;

	std::optional<int> maybe_integer(std::string_view column) const;
	std::optional<float> maybe_real(std::string_view column) const;
	std::optional<std::string> maybe_text(std::string_view column) const;

	char* raw(std::string_view column) const;
	int length(std::string_view column) const;
	bool exists(std::string_view column) const;
	bool is_null(std::string_view column) const;

private:

	int column_index(std::string_view column) const;

	PGresult* const result;
	const int row;

};

}
