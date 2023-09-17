#include "Buffer.hpp"

namespace frog {

Buffer::Buffer(size_type size) {
    allocate(size);
}

Buffer::Buffer(char* data, size_type size, Construction construction) {
    switch (construction) {
    case Construction::move:
        begin = data;
        end = begin + size;
        read_position = begin;
        write_position = end;
        break;
    case Construction::shallow_copy:
        begin = data;
        end = begin + size;
        read_position = begin;
        write_position = end;
        owner = false;
        break;
    }
}

Buffer::Buffer(const char* data, size_type size, ConstConstruction construction) {
    switch (construction) {
    case ConstConstruction::copy:
        write_raw(data, size);
        break;
    case ConstConstruction::shallow_copy:
        begin = const_cast<char*>(data); // note: don't worry -- never modified, because owner is false.
        end = begin + size;
        read_position = begin;
        write_position = end;
        owner = false;
        break;
    }
}

Buffer::Buffer(Buffer&& that) noexcept {
    std::swap(begin, that.begin);
    std::swap(end, that.end);
    std::swap(read_position, that.read_position);
    std::swap(write_position, that.write_position);
    std::swap(owner, that.owner);
}

Buffer::~Buffer() {
    if (owner) {
        delete[] begin;
    }
}

Buffer& Buffer::operator=(Buffer&& that) noexcept {
    std::swap(begin, that.begin);
    std::swap(end, that.end);
    std::swap(read_position, that.read_position);
    std::swap(write_position, that.write_position);
    std::swap(owner, that.owner);
    return *this;
}

void Buffer::allocate(size_type size) {
    if (begin) {
        resize(size);
        return;
    }
    try {
        begin = new char[size];
    } catch (const std::bad_alloc& e) {
        begin = nullptr;
    }
    end = begin;
    if (begin) {
        end += size;
    }
    read_position = begin;
    write_position = begin;
}

void Buffer::resize(size_type new_size) {
    if (!begin) {
        allocate(new_size);
        return;
    }
    const auto old_read_index = read_index();
    const auto old_write_index = write_index();
    char* old_begin{ begin };
    const auto old_size = size();
    const auto copy_size = std::min(old_size, new_size);
    begin = new char[new_size];
    std::memcpy(begin, old_begin, copy_size);
    if (owner) {
        delete[] old_begin;
    } else {
        owner = true;
    }
    end = begin;
    if (begin) {
        end += new_size;
    }
    read_position = begin + old_read_index;
    write_position = begin + old_write_index;
}

void Buffer::resize_if_needed(size_type size_to_write) {
    if (size_to_write > size_left_to_write()) {
        resize(size() * 2 + size_to_write + 64);
    }
}

void Buffer::set_read_index(size_type index) {
    if (index >= size()) {
        read_position = end;
    } else {
        read_position = begin + index;
    }
}

void Buffer::set_write_index(size_type index) {
    if (index >= size()) {
        write_position = end;
    } else {
        write_position = begin + index;
    }
}

void Buffer::move_read_index(long long size) {
    const auto index = static_cast<long long>(read_index()) + size;
    set_read_index(static_cast<std::size_t>(index));
}

void Buffer::move_write_index(long long size) {
    const auto index = static_cast<long long>(write_index()) + size;
    set_write_index(static_cast<std::size_t>(index));
}

bool Buffer::empty() const {
    return write_position == read_position;
}

Buffer::size_type Buffer::size() const {
    return static_cast<size_type>(end - begin);
}

Buffer::size_type Buffer::size_left_to_write() const {
    return static_cast<size_type>(end - write_position);
}

Buffer::size_type Buffer::size_left_to_read() const {
    return static_cast<size_type>(write_position - read_position);
}

Buffer::size_type Buffer::read_index() const {
    return static_cast<size_type>(read_position - begin);
}

Buffer::size_type Buffer::write_index() const {
    return static_cast<size_type>(write_position - begin);
}

char* Buffer::at_read() const {
    return read_position;
}

char* Buffer::at_write() const {
    return write_position;
}

void Buffer::write_raw(std::string_view buffer) {
    write_raw(buffer.data(), buffer.size());
}

void Buffer::write_raw(const void* source, size_type size) {
    resize_if_needed(size);
    std::memcpy(write_position, source, size);
    write_position += size;
}

void Buffer::read_raw(void* destination, size_type size) {
    if (end >= read_position + size) {
        std::memcpy(destination, read_position, size);
        read_position += size;
    }
}

char* Buffer::data() const {
    return begin;
}

void Buffer::reset() {
    read_position = begin;
    write_position = begin;
}

}
