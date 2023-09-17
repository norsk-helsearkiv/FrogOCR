#pragma once

#include <cmath>

namespace frog {

struct Quad {
    float x1{};
    float y1{};
    float x2{};
    float y2{};
    float x3{};
    float y3{};
    float x4{};
    float y4{};

    float bottomRightToLeftAngle() const {
        return std::atan((y3 - y4) / (x3 - x4));
    }

    float left() const {
        return std::min(x1, std::min(x2, std::min(x3, x4)));
    }

    float top() const {
        return std::min(y1, std::min(y2, std::min(y3, y4)));
    }

    float right() const {
        return std::max(x1, std::max(x2, std::max(x3, x4)));
    }

    float bottom() const {
        return std::max(y1, std::max(y2, std::max(y3, y4)));
    }

    float width() const {
        return right() - left();
    }

    float height() const {
        return bottom() - top();
    }

    bool contains(float x, float y) const {
        int crossCount{};
        crossCount += (((y1 <= y && y2 > y) || (y1 > y && y2 <= y)) && (x < (y - y1) * (x2 - x1) / (y2 - y1) + x1));
        crossCount += (((y2 <= y && y3 > y) || (y2 > y && y3 <= y)) && (x < (y - y2) * (x3 - x2) / (y3 - y2) + x2));
        crossCount += (((y3 <= y && y4 > y) || (y3 > y && y4 <= y)) && (x < (y - y3) * (x4 - x3) / (y4 - y3) + x3));
        crossCount += (((y4 <= y && y1 > y) || (y4 > y && y1 <= y)) && (x < (y - y4) * (x1 - x4) / (y1 - y4) + x4));
        return crossCount % 2 == 1;
    }
};

}
