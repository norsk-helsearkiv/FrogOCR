#pragma once

#include "Node.hpp"

#include <optional>
#include <string>
#include <vector>
#include <filesystem>
#include <functional>

namespace frog::xml {

class Document {
public:

	Document(std::string_view xml);
	Document(const Document&) = delete;
	Document(Document&&) = delete;

	~Document();

	Document& operator=(const Document&) = delete;
	Document& operator=(Document&&) = delete;

	Node getRootNode();

private:

	void* document{};

};

}
