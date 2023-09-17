#include "AltoMerger.hpp"
#include "Core/Filesystem.hpp"
#include "Core/Log.hpp"
#include "Alto/WriteXml.hpp"

namespace frog::alto {

AltoMerger::AltoMerger(std::vector<std::filesystem::path> input_paths, std::filesystem::path output_path)
	: input_paths{ std::move(input_paths) }, output_path{ std::move(output_path) } {
	load();
	for (auto& page : output_alto.layout.pages) {
		for (auto& printSpace : page.printSpaces) {
			mergePrintSpace(printSpace);
		}
	}
}

void AltoMerger::load() {
	if (input_paths.empty()) {
		return;
	}
	output_alto = { input_paths[0] };
	for (std::size_t i{ 1 }; i < input_paths.size(); i++) {
		input_altos.emplace_back(input_paths[i]);
	}
}

Alto& AltoMerger::getAlto() {
	return output_alto;
}

void AltoMerger::save() {
	write_file(output_path, to_xml(output_alto));
}

std::vector<const String*> AltoMerger::findCandidates(const TextLine& otherTextLine, const String& otherString) {
	const auto x = otherString.hpos;
	const auto y = otherTextLine.getMinimumVerticalPosition();
	const auto width = otherString.width;
	const auto height = otherTextLine.getMaxHeight();
	std::vector<const String*> strings;
	for (auto& input_alto : input_altos) {
		for (auto& page : input_alto.layout.pages) {
			for (auto& printSpace : page.printSpaces) {
				for (auto string : printSpace.getStringsInArea(x, y, width, height)) {
					if (string->confidence > otherString.confidence) {
						strings.push_back(string);
					}
				}
			}
		}
	}
	return strings;
}

void AltoMerger::mergePrintSpace(PrintSpace& printSpace) {
    // todo: fix file encoding
	std::u8string_view nordicLetters[]{
		u8"�",
		u8"�", 
		u8"�", 
		u8"�",
		u8"�",
		u8"�",
		u8"�",
		u8"�"
	};
	for (auto& composedBlock : printSpace.composedBlocks) {
		for (auto& textBlock : composedBlock.textBlocks) {
			for (auto& textLine : textBlock.textLines) {
				for (auto& string : textLine.strings) {
					const auto content = string.content;
					bool nordic{ false };
					for (auto letter : nordicLetters) {
						if (content.find(u8string_to_string(letter)) != std::string::npos) {
							nordic = true;
							break;
						}
					}
					if (nordic) {
						continue;
					}
					const String* best_candidate = &string;
					std::size_t best_distance{ 1000 };
					for (const auto& candidate : findCandidates(textLine, string)) {
						const auto distance = levenshtein(string.content, candidate->content);
						if (best_distance > distance && distance <= string.content.size() / 2) {
							best_distance = distance;
							if (candidate->confidence > best_candidate->confidence) {
								best_candidate = candidate;
							}
						}
					}
					if (best_distance < 1000 && best_candidate != &string) {
						log::info(R"("{}" WC {}% replaced with "{}" WC {}% (distance: {}))", string.content, string.confidence, best_candidate->content, best_candidate->confidence, best_distance);
						string.content = best_candidate->content;
						string.confidence = best_candidate->confidence;
					}
				}
			}
		}
	}
	for (auto& input_alto : input_altos) {
		for (auto& page : input_alto.layout.pages) {
			for (auto& print_space : page.printSpaces) {
				for (auto& composed_block : print_space.composedBlocks) {
					for (auto& text_block : composed_block.textBlocks) {
						for (auto& textLine : text_block.textLines) {
							for (auto& string : textLine.strings) {
								if (string.confidence < 70.0f) {
									continue;
								}
								if (string.content.empty() || string.content == " ") {
									continue;
								}
								const auto areaX = string.hpos;
								const auto areaY = textLine.getMinimumVerticalPosition();
								const auto areaWidth = string.width;
								const auto areaHeight = textLine.getMaxHeight();
								const int margin{ 50 };
								if (printSpace.getStringsInArea(areaX - margin, areaY - margin, areaWidth + margin * 2, areaHeight + margin * 2).empty()) {
									const auto hpos = string.hpos;
									const auto stringWidth = string.width;
									const auto stringHeight = string.height;
									if (auto closestLine = printSpace.findClosestTextLine(areaX, areaY, areaWidth, areaHeight, 10)) {
										const auto vpos = closestLine->getMinimumVerticalPosition();
										closestLine->strings.emplace_back(string.content, string.confidence, hpos, vpos, stringWidth, closestLine->getMaxHeight(), 0, std::vector<Glyph>{}, std::nullopt);
										log::info("Added \"{}\" WC {} to approximate line", string.content, string.confidence);
									} else {
										printSpace.composedBlocks.emplace_back(TextBlock{ String{ string.content, string.confidence, hpos, string.vpos, stringWidth, stringHeight, 0, {}, std::nullopt } });
										log::info("Added \"{}\" WC {} to new line", string.content, string.confidence);
									}
									merged_strings++;
								}
							}
						}
					}
				}
			}
		}
	}
}

std::vector<std::string_view> get_confident_words(const std::filesystem::path& xmlPath, float minConfidence) {
	std::vector<std::string_view> words;
	Alto alto{ xmlPath };
	for (const auto& page : alto.layout.pages) {
		for (const auto& printSpace : page.printSpaces) {
			for (const auto& composedBlock : printSpace.composedBlocks) {
				for (const auto& textBlock : composedBlock.textBlocks) {
					for (const auto& textLine : textBlock.textLines) {
						for (const auto& string : textLine.strings) {
							if (string.confidence >= minConfidence) {
								words.push_back(string.content);
							}
						}
					}
				}
			}
		}
	}
	return words;
}

}
