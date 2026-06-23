#pragma once
#include "endian.hpp"
#include "span.hpp"
#include "types.hpp"

namespace msl::streaming {
    using namespace msl::types;

    using InnerWrite = void (*)(const u8 *bytes, usize amount);
    using InnerRead = void (*)(u8 *bytes, usize *amount);

    template<usize buf_size>
    class Reader {
    public:
        Reader(const InnerRead read_fn, Span<u8, buf_size> buf) : inner_read(read_fn), buf(buf) {
        }

    private:
        InnerRead inner_read;

        Span<u8, buf_size> buf;
    };

    template<usize buf_size>
    class Writer {
    public:
        Writer(const InnerWrite write_fn, Span<u8, buf_size> buffer) : inner_write(write_fn), buf(buffer) {
        }

        template<typename T, usize size, bytes::Endian endianness = bytes::NativeEndian> requires concepts::integral<T>
        void write(Span<const T, size> values) {
            constexpr usize size_bytes = sizeof(T) * size;

            // ReSharper disable once CppTooWideScope
            constexpr bool need_swap = endianness != bytes::NativeEndian && sizeof(T) > 1;
            T raw_buf[need_swap ? size : 1];

            if constexpr (need_swap) {
                for (usize i = 0; i < size; ++i) {
                    raw_buf[i] = bytes::swap(values[i]);
                }
            }

            auto raw = Span<const u8, size_bytes>(
                reinterpret_cast<const u8 *>(need_swap ? raw_buf : values.ptr));
            writeRawBytes(raw);
        }

        template<typename T, bytes::Endian endianness = bytes::NativeEndian> requires concepts::integral<T>
        void write(Span<const T> values) {
            // ReSharper disable once CppTooWideScope
            constexpr bool need_swap = endianness != bytes::NativeEndian && sizeof(T) > 1;

            if constexpr (need_swap) {
                constexpr usize local_buffer_size = 2048;
                T cache[local_buffer_size];
                usize written = 0;

                const usize full_buffers_needed = values.len / local_buffer_size; // Скільки повних буферів можна записати
                const usize remainder = values.len % local_buffer_size; // скільки елементів лишається

                for (usize i = 0; i < full_buffers_needed; ++i) {
                    for (usize j = 0; j < local_buffer_size; ++j) {
                        cache[j] = byteswap(values[written + j]);
                    }
                    const auto raw = Span<const u8, local_buffer_size * sizeof(T)>(
                        reinterpret_cast<const u8*>(cache));
                    writeRawBytes(raw);
                    written += local_buffer_size;
                }
                for (usize i = 0; i < remainder; ++i) {
                    cache[i] = byteswap(values[written + i]);
                }
                const auto raw = Span<const u8>(reinterpret_cast<const u8*>(cache), remainder * sizeof(T));
                writeRawBytes(raw);
                written += remainder;
            } else {
                const auto raw = Span<const u8>(reinterpret_cast<const u8*>(values.ptr),
                                                      values.size_bytes());
                writeRawBytes(raw);
            }
        }

        template<typename T, bytes::Endian endianness = bytes::NativeEndian> requires concepts::integral<T>
        void writeInt(T value) {
            if (occupancy + sizeof(T) > buf.len) flush();
            if constexpr (endianness != bytes::NativeEndian && sizeof(T) > 1) {
                value = bytes::swap(value);
            }
            memcpy(this->buf.shift_start(occupancy).ptr, reinterpret_cast<const u8*>(&value), sizeof(T));
            occupancy += sizeof(T);
        }

        void flush() {
            inner_write(buf.ptr, occupancy);
            occupancy = 0;
        }

    private:
        InnerWrite inner_write;

        Span<u8, buf_size> buf;
        usize occupancy = 0;

        void writeRawBytes(const Span<const u8> raw) {
            usize written_bytes = 0;
            while (written_bytes < raw.size_bytes()) {
                const usize to_write = min(buf.len - occupancy, raw.len - written_bytes);
                buf.shift_start(occupancy).copy(raw.shift_start(written_bytes));

                occupancy += to_write;
                written_bytes += to_write;

                if (occupancy == buf.len) flush();
            }
        }
    };
}
