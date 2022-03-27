#ifndef PARSBUFUTILS_HPP
#define PARSBUFUTILS_HPP

#include <list>
#include <string>
#include "typedefs.h"

class ParsBufUtils {
public:
    static std::list<std::wstring> BreakBufferIntoWstrings(byte * buf, qword len) {
        std::list<std::wstring> wstring_list;
        
        if (*((wchar_t*)(buf + len - 2)) != 0x0000) {
            return wstring_list;
        }

        qword start, end;
        start = end = 0;

        while (start + 2 < len) {
            while (end + 2 < len) {
                if (*((wchar_t*)(buf + end)) != L'\n' && *((wchar_t*)(buf + end)) != L'\r'  && *((wchar_t*)(buf + end)) != 0x0000) end+=2;
                else break;
            }
            *((wchar_t*)(buf + end)) = 0x0000;
            if (end - start >= 2)
                wstring_list.emplace_back((wchar_t *)(buf + start));
            start = end + 2;
            end = start;
        }

        return wstring_list;
    }
};

#endif // PARSBUFUTILS_HPP