#pragma once

#include <vector>
#include <string>

namespace frog::alto {
class Alto;
}

namespace frog::database {
class Connection;
}

namespace frog::postprocess {

class WordMatcher {
public:

	WordMatcher(database::Connection& database);

	void fix(alto::Alto& alto);

private:

	std::vector<std::pair<std::string, int>> words;

};

}
