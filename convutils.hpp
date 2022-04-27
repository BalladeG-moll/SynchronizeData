#ifndef CONVUTILS_HPP
#define CONVUTILS_HPP

#include <string>
#include <array>
#include "typedefs.h"

using byte = unsigned char;
using word = unsigned short;
using dword = unsigned int;
using qword = unsigned long long;

class ConvUtils {
public:
    static void ByteStr_to_UTF16LE(const byte* ByteStr, wchar_t* UTF16LE_str) {
        for (int i = 0; ByteStr[i] != 0; i++)
            if (ByteStr[i] <= 0x80)
                UTF16LE_str[i] = ByteStr[i];
    }

    template <int array_size> static std::wstring array_to_UTF16LE_wstring(const std::array<byte, array_size> & arr) {
        wchar_t buf[array_size * 2 + array_size];
        dword i = 0;
        for(auto &item: arr) {
            byte msn = item >> 4; //most significant number
            byte lsn = item & 0x0F; //least significant number
            buf[i] = (msn <= 9)? msn + 0x30 : msn + 0x37;
            buf[++i] = (lsn <= 9)? lsn + 0x30 : lsn + 0x37;
            buf[++i] = L' ';
            ++i;
        }
        buf[i - 1] = 0;
        return std::wstring(buf);
    }

    template <int array_size> static void UTF16LE_wstring_to_array(const std::wstring & s, std::array<byte, array_size> & arr) {
        for(dword i = 0, j = 0; j < s.length(); i++, j+=3) {
            byte msn = s[j]; //most significant number
            byte lsn = s[j + 1]; //least significant number
            if (msn >= 0x30 && msn <= 0x39)
                arr[i] = (msn - 0x30) << 4;
            else if(msn >= 0x41 && msn <= 0x46)
                arr[i] = (msn - 0x37) << 4;

            if (lsn >= 0x30 && lsn <= 0x39)
                arr[i] |= (lsn - 0x30);
            else if(msn >= 0x41 && msn <= 0x46)
                arr[i] |= (lsn - 0x37);
        }
    }
};

#endif // CONVUTILS_HPP
