#pragma once
#include "bytes.hpp"
#include "option.hpp"
#include "result.hpp"
#include "span.hpp"
#include "types.hpp"

namespace msl::io {
    using namespace msl::types;

    template<usize buf_size>
    class Reader {
    public:
        explicit Reader(Span<u8, buf_size> buf) : buf(buf) {
        }

        enum class ReadErr : u8 {
            SmallBuffer,
        };

        /**
         * @brief Reads a fixed-size span of integral elements from the buffer.
         * Performs a fast linear block copy followed by an in-place byte swap
         * if the target endianness differs from the system native byte order.
         * @tparam T Integral type of the elements.
         * @tparam size Compile-time static capacity of the span.
         * @tparam endianness Target byte order to interpret from the packet.
         * @param values Static Span pointing to the destination memory block.
         * @return Result<usize, ReadErr> Number of BYTES read on success, or ReadErr if out of bounds.
         */
        template<concepts::integral T, usize size, bytes::Endian endianness = bytes::NativeEndian>
        Result<usize, ReadErr> read(Span<T, size> values) {
            if (buf.size_bytes() - read_bytes < values.size_bytes()) {
                return Err(ReadErr::SmallBuffer);
            }
            const usize bytes_to_read = values.size_bytes();
            mem::copy(values.ptr, buf.ptr + read_bytes, bytes_to_read);
            for (usize i = 0; i < size; ++i) {
                if constexpr (bytes::need_swap<T, endianness>()) {
                    values[i] = bytes::swap(values[i]);
                }
            }
            read_bytes += size * sizeof(T);
            return Ok(bytes_to_read);
        }

        /**
         * @brief Reads a dynamic runtime span of integral elements from the buffer.
         * Copies a continuous block of bytes into the destination span and applies
         * conditional vectorizable byte-swapping based on compile-time endianness checks.
         * @tparam T Integral type of the elements.
         * @tparam endianness Target byte order to interpret from the packet.
         * @param values Dynamic Span pointing to the destination memory block.
         * @return Result<usize, ReadErr> Number of BYTES read on success, or ReadErr if out of bounds.
         */
        template<concepts::integral T, bytes::Endian endianness = bytes::NativeEndian>
        Result<usize, ReadErr> read(Span<T> values) {
            if (buf.size_bytes() - read_bytes < values.size_bytes()) {
                return Err(ReadErr::SmallBuffer);
            }
            const usize bytes_to_read = values.size_bytes();
            mem::copy(values.ptr, buf.ptr + read_bytes, bytes_to_read);
            for (usize i = 0; i < values.len; ++i) {
                if constexpr (bytes::need_swap<T, endianness>()) {
                    values[i] = bytes::swap(values[i]);
                }
            }
            read_bytes += values.len * sizeof(T);
            return Ok(bytes_to_read);
        }

        /**
         * @brief Reads a single scalar integer from the buffer, returning it via Result wrapper.
         * Monadic-style API for sequential parsing. Safe against out-of-bounds reads.
         * @tparam T Integral type to read.
         * @tparam endianness Target byte order of the integer in the buffer.
         * @return Result<T, ReadErr> The parsed scalar value, or ReadErr if the buffer is exhausted.
         */
        template<concepts::integral T, bytes::Endian endianness = bytes::NativeEndian>
        Result<T, ReadErr> readInt() {
            if (buf.size_bytes() - read_bytes < sizeof(T)) {
                return Err(ReadErr::SmallBuffer);
            }
            T value;
            mem::copy(&value, buf.ptr + read_bytes, sizeof(T));

            if constexpr (bytes::need_swap<T, endianness>()) {
                value = bytes::swap(value);
            }
            read_bytes += sizeof(T);
            return Ok(value);
        }

        /**
         * @brief Reads a single scalar integer directly into the provided destination pointer.
         * @tparam T Integral type to read.
         * @tparam endianness Target byte order of the integer in the buffer.
         * @param out Valid non-null pointer to memory where the result will be written.
         * @return Option<ReadErr> Error code if read failed, or `none` if operation succeeded.
         * Nullptr passed to out is defined as undefined behaviour. (nullptr dereference)
         */
        template<concepts::integral T, bytes::Endian endianness = bytes::NativeEndian>
        Option<ReadErr> readInt(T *out) {
            if (buf.size_bytes() - read_bytes < sizeof(T)) {
                return Value(ReadErr::SmallBuffer);
            }
            mem::copy(out, buf.ptr + read_bytes, sizeof(T));

            if constexpr (bytes::need_swap<T, endianness>()) {
                *out = bytes::swap(*out);
            }
            read_bytes += sizeof(T);
            return none;
        }
    private:
        usize read_bytes = 0;
        Span<u8, buf_size> buf;
    };

    template<usize buf_size>
    class Writer {
    public:
        explicit Writer(Span<u8, buf_size> buffer) : buf(buffer) {
        }

        enum class WriteErr : u8 {
            BufferIsFull
        };

        /**
         * @brief Writes a fixed-size (compile-time) array into the buffer.
         * If byte swapping is required, a safe local array is allocated on the stack.
         * Due to the fixed size, the compiler can fully unroll the loop and
         * auto-vectorize the byte swapping operation.
         * @tparam T Integral type of elements (concepts::integral).
         * @tparam size Number of elements in the array (compile-time constant).
         * @tparam endianness Byte order of the target packet (defaults to NativeEndian).
         * @param values Fixed-size Span pointing to the source data.
         * @return Result<usize, WriteErr> Number of bytes written, or BufferIsFull error.
         */
        template<concepts::integral T, usize size, bytes::Endian endianness = bytes::NativeEndian>
        Result<usize, WriteErr> write(Span<const T, size> values) {
            constexpr usize size_bytes = sizeof(T) * size;

            constexpr bool need_swap = bytes::need_swap<T, endianness>();
            T raw_buf[need_swap ? size : 1];

            if constexpr (need_swap) {
                for (usize i = 0; i < size; ++i) {
                    raw_buf[i] = bytes::swap(values[i]);
                }
            }

            auto raw = Span<const u8, size_bytes>(
                reinterpret_cast<const u8 *>(need_swap ? raw_buf : values.ptr));
            return writeRawBytes(raw.make_dynamic());
        }

        [[nodiscard]] bool is_full() const noexcept {
            return occupancy == buf_size;
        }

        [[nodiscard]] usize available() const noexcept {
            return buf_size - occupancy;
        }

        /**
         * @brief Writes a dynamic (runtime) array into the buffer using streaming logic.
         * * If the entire array does not fit, the method writes as many FULL elements of
         * type T as the remaining space allows, implementing "write as much as possible" behavior.
         * * @tparam T Integral type of elements.
         * @tparam endianness Byte order of the target packet.
         * @param values Dynamic Span pointing to the source data.
         * @return Result<usize, WriteErr> Number of BYTES actually written, or error if nothing fits.
         */
        template<concepts::integral T, bytes::Endian endianness = bytes::NativeEndian>
        Result<usize, WriteErr> write(Span<const T> values) {
            const usize available_bytes = buf.len - occupancy;
            const usize requested_bytes = values.len * sizeof(T);
            const usize to_write_bytes = requested_bytes < available_bytes ? requested_bytes : available_bytes;

            const usize elems_to_write = to_write_bytes / sizeof(T);
            if (elems_to_write == 0 && values.len > 0) return Err(WriteErr::BufferIsFull);

            const usize bytes_to_write = elems_to_write * sizeof(T);
            u8* const dest_bytes = buf.ptr + occupancy;

            mem::copy(dest_bytes, reinterpret_cast<const u8*>(values.ptr), bytes_to_write);

            if constexpr (bytes::need_swap<T, endianness>()) {
                T* const dest_elements = reinterpret_cast<T*>(dest_bytes);
                for (usize i = 0; i < elems_to_write; ++i) {
                    dest_elements[i] = bytes::swap(dest_elements[i]);
                }
            }

            occupancy += bytes_to_write;
            return Ok(bytes_to_write);
        }

        /**
         * @brief Writes a single integral scalar value into the buffer.
         * @tparam T Integral type (u8, u16, u32, u64, etc.).
         * @tparam endianness Target byte order.
         * @param value The scalar value to write.
         * @return Result<usize, WriteErr> Size of type T on success, or error.
         */
        template<concepts::integral T, bytes::Endian endianness = bytes::NativeEndian>
        Result<usize, WriteErr> writeInt(T value) {
            if (occupancy + sizeof(T) > buf.len) [[unlikely]] return Err(WriteErr::BufferIsFull);
            if constexpr (bytes::need_swap<T, endianness>()) {
                value = bytes::swap(value);
            }
            mem::copy(buf.shift_start(occupancy).ptr, reinterpret_cast<const u8 *>(&value), sizeof(T));
            occupancy += sizeof(T);
            return Ok(sizeof(T));
        }

    private:
        Span<u8, buf_size> buf;
        usize occupancy = 0;

        Result<usize, WriteErr> writeRawBytes(Span<const u8> raw) {
            const usize to_write = buf.len - occupancy < raw.len ? buf.len - occupancy : raw.len;
            mem::copy(buf.ptr + occupancy, raw.ptr, to_write);
            occupancy += to_write;
            if (occupancy == buf.len) return Err(WriteErr::BufferIsFull);
            return Ok(to_write);
        }
    };
}
