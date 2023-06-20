#include "WordMatcher.hpp"
#include "Alto/Alto.hpp"
#include "Core/Log.hpp"
#include "Core/Database/Connection.hpp"

namespace frog::postprocess {

WordMatcher::WordMatcher(database::Connection& database) {
	if (auto result = database.execute("select * from word order by frequency desc")) {
		for (int i{ 0 }; i < result.count(); i++) {
			auto row = result.row(i);
			words.emplace_back(row.text_view("value"), row.integer("frequency"));
		}
	}
}

void WordMatcher::fix(alto::Alto& alto) {
	for (auto& page : alto.layout.pages) {
		for (auto& printSpace : page.printSpaces) {
			for (auto& composedBlock : printSpace.composedBlocks) {
				for (auto& textBlock : composedBlock.textBlocks) {
					for (auto& textLine : textBlock.textLines) {
						for (auto& string : textLine.strings) {
							if (string.confidence < 0.3f) {
								continue; // too bad to expect good results.
							}
							if (string.confidence > 0.9f) {
								continue; // probably good enough already.
							}
							if (string.content == "." || string.content == ":" || string.content == "/") {
								continue; // turning these into , or ; or | is more likely to be a degradation.
							}
							int digitsFound{ 0 };
							for (const auto character : string.content) {
								if (character >= '0' && character <= '9') {
									digitsFound++;
								}
							}
							if (digitsFound >= 2) {
								continue;
							}

							std::vector<std::string> allWordVariants{ "" };
							for (const auto& glyph : string.glyphs) {
								const auto glyphVariantCount = static_cast<int>(glyph.variants.size()) + 1;
								auto baseWordVariants = allWordVariants;
								for (auto& wordVariant : allWordVariants) {
									wordVariant += glyph.content;
								}
								if (glyph.confidence >= 0.98f) {
									continue; // no point checking variants for such a good confidence.
								}
								for (const auto& variant : glyph.variants) {
									auto newWordVariants = baseWordVariants;
									for (auto& newWordVariant : newWordVariants) {
										newWordVariant += variant.content;
									}
									allWordVariants.insert(allWordVariants.begin(), newWordVariants.begin(), newWordVariants.end());
								}
							}

							std::string_view bestWord;
							for (const auto& wordVariant : allWordVariants) {
								for (const auto& [word, frequency] : words) {
									const auto distance = levenshtein(word, wordVariant);
									if (distance == 0) {
										bestWord = word;
										break;
									}
								}
								if (!bestWord.empty()) {
									break;
								}
							}
							
							if (bestWord.empty() || string.content == bestWord) {
								continue;
							}

							log::info("Changing {} to {}", string.content, bestWord);
						}
					}
				}
			}
		}
	}
}

}
