#pragma once

namespace frog::ocr {
struct Document;
}

namespace frog::postprocess {

void remove_garbage(ocr::Document& document);

}
