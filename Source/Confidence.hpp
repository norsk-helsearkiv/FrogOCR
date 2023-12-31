#pragma once

namespace frog {

class Confidence {
public:

	enum class Format { normalized, percent };

	Confidence(float confidence, Format format) {
		if (format == Format::percent) {
			normalizedConfidence = confidence / 100.0f;
		} else {
            normalizedConfidence = confidence;
        }
	}

	Confidence() = default;
	
	float getNormalized() const {
		return normalizedConfidence;
	}

	float getPercent() const {
		return normalizedConfidence * 100.0f;
	}

private:

	float normalizedConfidence{};

};

}
