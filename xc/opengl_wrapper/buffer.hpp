#pragma once
#include <vector>

#include "opengl_wrapper/types.hpp"

namespace xc::opengl {
class Buffer;
namespace details {

template <typename T>
struct BufferDataHelper {
    static void buffer(const Buffer& buffer, const T& data, BufferTarget target,
                       BufferUsage usage) {
        static_assert(false,
                      "BufferDataHelper<T> is not implemented for this type");
    }
};
}  // namespace details

class Buffer {
   public:
    Buffer(const Buffer&) = delete;
    Buffer(Buffer&&) = default;
    Buffer& operator=(const Buffer&) = delete;
    Buffer& operator=(Buffer&&) = default;

   public:
    Buffer();
    ~Buffer();

    void bind(BufferTarget type) const noexcept;
    static void unbind(BufferTarget type) noexcept;

    void buffer_data(void* data, size_t size, BufferTarget target,
                     BufferUsage usage) const noexcept;
    void buffer_sub_data(void* data, size_t size, BufferTarget target,
                         BufferUsage usage) const noexcept;

    void read(void* data, size_t size, BufferTarget target) const noexcept;

    template <typename T>
    void buffer_data(BufferTarget target, BufferUsage usage, const T& data) {
        details::BufferDataHelper<T>::buffer(*this, data, target, usage);
    }
    buffer_id_t id() const noexcept { return id_; }

   private:
    buffer_id_t id_;
};

namespace details {

template <typename T>
struct BufferDataHelper<std::vector<T>> {
    static void buffer(const Buffer& buffer, const std::vector<T>& data,
                       BufferTarget target, BufferUsage usage) {
        buffer.buffer_data(
            const_cast<void*>(static_cast<const void*>(data.data())),
            data.size() * sizeof(T), target, usage);
    }
};
template <typename T, size_t N>
struct BufferDataHelper<std::array<T, N>> {
    static void buffer(const Buffer& buffer, const std::array<T, N>& data,
                       BufferTarget target, BufferUsage usage) {
        buffer.buffer_data(
            const_cast<void*>(static_cast<const void*>(data.data())),
            data.size() * sizeof(T), target, usage);
    }
};

template <typename T, size_t N>
struct BufferDataHelper<T[N]> {
    static void buffer(const Buffer& buffer, const T (&data)[N],
                       BufferTarget target, BufferUsage usage) {
        buffer.buffer_data(const_cast<void*>(static_cast<const void*>(data)),
                           N * sizeof(T), target, usage);
    }
};
template <template <typename...> class Container, typename T>
struct BufferDataHelper<Container<T>> {
    static void buffer(const Buffer& buffer, const Container<T>& data,
                       BufferTarget target, BufferUsage usage) {
        std::vector<T> vec(data.begin(), data.end());
        buffer.buffer_data(
            const_cast<void*>(static_cast<const void*>(vec.data())),
            vec.size() * sizeof(T), target, usage);
    }
};

}  // namespace details

// 特化处理vector和array

}  // namespace xc::opengl
