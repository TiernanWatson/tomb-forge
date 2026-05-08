#pragma once

#include <istream>
#include <string>
#include <vector>
#include <cstdint>
#include <type_traits>
#include <cstring>
#include <limits>

#include "Core/Debug.h"

namespace TombForge
{
    /// Takes a binary stream and reads it into various types
    class BinaryReader
    {
    public:
        explicit BinaryReader(std::istream& stream)
            : m_stream(stream)
        {
        }

        uint8_t ReadUInt8() { return static_cast<uint8_t>(m_stream.get()); }
        int8_t ReadInt8() { return static_cast<int8_t>(m_stream.get()); }

        uint16_t ReadUInt16() { return ReadLE<uint16_t>(); }
        int16_t ReadInt16() { return ReadLE<int16_t>(); }
        uint32_t ReadUInt32() { return ReadLE<uint32_t>(); }
        int32_t ReadInt32() { return ReadLE<int32_t>(); }
        uint64_t ReadUInt64() { return ReadLE<uint64_t>(); }
        int64_t ReadInt64() { return ReadLE<int64_t>(); }
        float ReadFloat() { return ReadLE<float>(); }
        double ReadDouble() { return ReadLE<double>(); }
        bool ReadBool() { return ReadUInt8() != 0; }

        template<typename T>
        void ReadArray(T* outArray, size_t count)
        {
            m_stream.read(reinterpret_cast<char*>(outArray), sizeof(T) * count);
            if (!m_stream.good())
            {
                LOG_ERROR("Failed to read array of size %zu", count);
                return;
            }
            for (size_t i = 0; i < count; i++)
            {
                outArray[i] = FromLittleEndian(outArray[i]);
            }
        }

        std::string ReadString()
        {
            const uint32_t size = ReadUInt32();
            std::string str(size, '\0');
            m_stream.read(&str[0], size);
            return str;
        }

        template<typename T>
        std::vector<T> ReadVector()
        {
            const uint32_t size = ReadUInt32();

            std::vector<T> vec(size);
            for (uint32_t i = 0; i < size; ++i)
            {
                vec[i] = ReadLE<T>();
            }

            return vec;
        }

        void ReadBytes(void* data, size_t size)
        {
            m_stream.read(reinterpret_cast<char*>(data), size);
        }

        bool Good() const
        {
            return m_stream.good();
        }

    private:
        template<typename T>
        T ReadLE()
        {
            T value;
            m_stream.read(reinterpret_cast<char*>(&value), sizeof(T));
            return FromLittleEndian(value);
        }

        template<typename T>
        static T FromLittleEndian(T value)
        {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
            return value;
#else
            T result = 0;
            for (size_t i = 0; i < sizeof(T); i++)
            {
                reinterpret_cast<uint8_t*>(&result)[i] = reinterpret_cast<const uint8_t*>(&value)[sizeof(T) - 1 - i];
            }
            return result;
#endif
        }

        std::istream& m_stream;
    };
}
