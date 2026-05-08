#pragma once

#include <ostream>
#include <string>
#include <vector>
#include <cstdint>
#include <type_traits>
#include <cstring>

#include "Core/Debug.h"

namespace TombForge
{
    /// Takes a stream and writes binary data to it in little-endian format
    class BinaryWriter
    {
    public:
        explicit BinaryWriter(std::ostream& stream)
            : m_stream(stream)
        {
        }

        void WriteUInt8(uint8_t value) { m_stream.put(static_cast<char>(value)); }
        void WriteInt8(int8_t value) { m_stream.put(static_cast<char>(value)); }

        void WriteUInt16(uint16_t value) { WriteLE(value); }
        void WriteInt16(int16_t value) { WriteLE(value); }
        void WriteUInt32(uint32_t value) { WriteLE(value); }
        void WriteInt32(int32_t value) { WriteLE(value); }
        void WriteUInt64(uint64_t value) { WriteLE(value); }
        void WriteInt64(int64_t value) { WriteLE(value); }
        void WriteFloat(float value) { WriteLE(value); }
        void WriteDouble(double value) { WriteLE(value); }
        void WriteBool(bool value) { WriteUInt8(value ? 1 : 0); }

        void WriteString(const std::string& str)
        {
            WriteUInt32(static_cast<uint32_t>(str.size()));
            m_stream.write(str.data(), str.size());
        }

        template<typename T>
        void WriteArray(const T* array, size_t count)
        {
            for (size_t i = 0; i < count; i++)
            {
                WriteLE(array[i]);
            }
        }

        template<typename T>
        void WriteVector(const std::vector<T>& vec)
        {
            ASSERT(vec.size() <= std::numeric_limits<uint32_t>::max(), "Trying to write array that is larger than max size");

            WriteUInt32(static_cast<uint32_t>(vec.size()));

            for (const T& elem : vec)
            {
                WriteLE(elem);
            }
        }

        void WriteBytes(const void* data, size_t size)
        {
            m_stream.write(reinterpret_cast<const char*>(data), size);
        }

        bool Good() const
        {
            return m_stream.good();
        }

    private:
        template<typename T>
        void WriteLE(T value)
        {
            T leValue = ToLittleEndian(value);
            m_stream.write(reinterpret_cast<const char*>(&leValue), sizeof(T));
        }

        template<typename T>
        static T ToLittleEndian(T value)
        {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
            return value;
#else
            T result = 0;
            for (size_t i = 0; i < sizeof(T); i++)
            {
                // Note: for structs/aggregates, the last element will end up first
                reinterpret_cast<uint8_t*>(&result)[i] = reinterpret_cast<const uint8_t*>(&value)[sizeof(T) - 1 - i];
            }

            return result;
#endif
        }

        std::ostream& m_stream;
    };
}
