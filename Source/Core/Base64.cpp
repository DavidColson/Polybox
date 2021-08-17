#include "Base64.h"

#include <format>

// ***********************************************************************

std::string printBits(int size, void const * const ptr)
{
    // Little endian
    std::string out;
    unsigned char *b = (unsigned char*) ptr;
    unsigned char byte;
    int i, j;

    for (i = size - 1; i >= 0; i--)
    {
        for (j = 7; j >= 0; j--)
        {
            byte = (b[i] >> j) & 1;
            out += std::format("{:d}", byte);
        }
    }
    return out;
}

static const unsigned char encodingTable[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// ***********************************************************************

std::string DecodeBase64(std::string const& encodedString)
{
    unsigned char decodingTable[256];

    for (size_t i = 0; i < sizeof(encodingTable) - 1; i++)
        decodingTable[encodingTable[i]] = (unsigned char) i;
    decodingTable['='] = 0;

    const char* input = encodedString.data();
    size_t inputLength = encodedString.length();

    size_t count = 0;
    for (int i = 0; i < inputLength; i++) {
        if (decodingTable[input[i]] != 0x80)
            count++;
    }

    if (count == 0 || count % 4)
    {
        //Log::Crit("Invalid base64 encoded string");
        return NULL;
    }

    size_t outputlen = count / 4 * 3;

    char* output = new char[outputlen];
    char* position = output;

    for( int i= 0; i < inputLength; i += 4)
    {
        char a = decodingTable[input[i]] & 0xFF;
        char b = decodingTable[input[i + 1]] & 0xFF;
        char c = decodingTable[input[i + 2]] & 0xFF; 
        char d = decodingTable[input[i + 3]] & 0xFF; 

        *position++ = a << 2 | (b & 0x30) >> 4;
        if (c != 0x40)
            *position++ = b << 4 | (c & 0x3c) >> 2;
        if (d != 0x40)
            *position++ = (c << 6 | d);
    }

    std::string result(output, output + outputlen);
    delete output;
    return result;
}

// ***********************************************************************

std::string EncodeBase64(size_t length, const char* bytes)
{
    size_t outputLength = length * 4 / 3 + 4; // 3-byte blocks to 4-byte
    outputLength++; // nul termination
    if (outputLength < length)
        return NULL; // integer overflow
    char* output = new char[outputLength];
    char* position = output;

    for( int i= 0; i < length; i += 3)
    {
        int nChars = (int)length - i;
        char a = bytes[i];
        char b = nChars == 1 ? 0xF : bytes[i + 1];
        char c = nChars < 3 ? 0x3F : bytes[i + 2]; 
        
        *position++ = encodingTable[a >> 2];
        *position++ = encodingTable[(a & 0x3) << 4 | b >> 4];
        *position++ = encodingTable[b == 0xF ? 0x40 : (b & 0xF) << 2 | c >> 6];
        *position++ = encodingTable[c == 0x3F ? 0x40 : (c & 0x3F)];
    }

    std::string result(output, output + outputLength);
    delete output;
    return result;
}
