#pragma once

#include "Alto/Alto.hpp"
#include "Alto/String.hpp"
#include "Alto/Page.hpp"

#include <vector>
#include <filesystem>

namespace frog::alto {

class AltoMerger {
public:

	AltoMerger(std::vector<std::filesystem::path> input_paths, std::filesystem::path output_path);
	AltoMerger(const AltoMerger&) = delete;
	AltoMerger(AltoMerger&&) = delete;

	~AltoMerger() = default;

	AltoMerger& operator=(const AltoMerger&) = delete;
	AltoMerger& operator=(AltoMerger&&) = delete;

	Alto& getAlto();

	void save();

private:

	void load();
	void mergePrintSpace(PrintSpace& printSpace);

	std::vector<const String*> findCandidates(const TextLine& textLine, const String& string);

	std::vector<std::filesystem::path> input_paths;
	std::filesystem::path output_path;

	std::vector<Alto> input_altos;
	Alto output_alto;

	int merged_strings{ 0 };

};

std::vector<std::string_view> get_confident_words(const std::filesystem::path& xml_path, float min_confidence);

}
