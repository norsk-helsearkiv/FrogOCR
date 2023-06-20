#pragma once

#include <filesystem>
#include <optional>

namespace frog::xml {

class Validator {
public:

    Validator(std::filesystem::path schema_path);
    Validator(const Validator&) = delete;
    Validator(Validator&&) = delete;

    ~Validator();

    Validator& operator=(const Validator&) = delete;
    Validator& operator=(Validator&&) = delete;

    [[nodiscard]] std::optional<std::string> validate(std::string_view xml) const;

private:

    void* schema_handle{};
    void* valid_schema_context_handle{};
    const std::filesystem::path schema_path;

};

}
