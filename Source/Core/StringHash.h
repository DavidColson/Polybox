#pragma once

// Implements compile time string hashing.
// Forgive my academic masturbation here but this is a damn cool thing to have,
struct Fnv1a
{
    constexpr static inline uint32_t Hash(char const*const aString, const uint32_t val = 0x811C9DC5)
    {
        return (aString[0] == '\0') ? val : Hash( &aString[1], ( val ^ uint32_t(aString[0]) ) * 0x01000193);
    }

    constexpr static inline uint32_t Hash(char const*const aString, const size_t aStrlen, const uint32_t val = 0x811C9DC5)
    {
        return (aStrlen == 0) ? val : Hash( aString + 1, aStrlen - 1, ( val ^ uint32_t(aString[0]) ) * 0x01000193);
    }
};

inline constexpr uint32_t operator "" _hash (const char* aString, const size_t aStrlen)
{
	return Fnv1a::Hash(aString, aStrlen);
}