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
    static void CP1251_to_UTF16LE(const byte* CP12521_str, wchar_t* UTF16LE_str) {
        for (int i = 0; CP12521_str[i] != 0; i++) {
            if (CP12521_str[i] <= 0x80)
                UTF16LE_str[i] = CP12521_str[i];
            else if (CP12521_str[i] >= 0xC0 && CP12521_str[i] <= 0xDF || CP12521_str[i] >= 0xE0 && CP12521_str[i] <= 0xFF)
                UTF16LE_str[i] = 0x0400 | CP12521_str[i] - 0xB0;
            else if (CP12521_str[i] == 0xB8)
                UTF16LE_str[i] = 0x0451;
            else if (CP12521_str[i] == 0xA8)
                UTF16LE_str[i] = 0x0401;
        }
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
