#pragma once

#include <filesystem>
#include <vector>
#include <optional>
#include <algorithm>
#include <cstring>
#include <cmath>

namespace frog {

class Buffer {
public:

    using size_type = std::uintmax_t;

    char* begin{ nullptr };
    char* end{ nullptr };
    char* read_position{ nullptr };
    char* write_position{ nullptr };
    bool owner{ true };

    enum class Construction { move, shallow_copy };
    enum class ConstConstruction { copy, shallow_copy };

    Buffer() = default;
    Buffer(size_type size);
    Buffer(char* data, size_type size, Construction construction);
    Buffer(const char* data, size_type size, ConstConstruction construction);
    Buffer(const Buffer&) = delete;
    Buffer(Buffer&&) noexcept;

    ~Buffer();

    Buffer& operator=(const Buffer&) = delete;
    Buffer& operator=(Buffer&&) noexcept;

    std::string_view get_string_view() const {
        return { at_read(), static_cast<std::string_view::size_type>(size_left_to_read()) };
    }

    std::string_view get_string_view(size_type from, size_type to) const {
        return { at_read() + from, static_cast<std::string_view::size_type>(to - from) };
    }

    void allocate(size_type size);
    void resize(size_type new_size);
    void resize_if_needed(size_type size_to_write);

    void set_read_index(size_type index);
    void set_write_index(size_type index);

    void move_read_index(long long size);
    void move_write_index(long long size);

    [[nodiscard]] bool empty() const;
    [[nodiscard]] size_type size() const;
    [[nodiscard]] size_type size_left_to_write() const;
    [[nodiscard]] size_type size_left_to_read() const;
    [[nodiscard]] size_type read_index() const;
    [[nodiscard]] size_type write_index() const;

    char* at_read() const;
    char* at_write() const;

    void write_raw(std::string_view buffer);
    void write_raw(const void* source, size_type size);
    void read_raw(void* destination, size_type size);

    char* data() const;
    void reset();

};

}