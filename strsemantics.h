#ifndef STRSEMANTICS_H
#define STRSEMANTICS_H
#include <string>
#include "typedefs.h"

class StrSemantics {
public:
    static bool isHash(std::wstring& s, const dword hash_len) {
        if (s.length() != hash_len * 2 + hash_len - 1) return false;
        byte spaces = 0;
        for (byte i = 0; i < s.length(); i++) {
            if (s[i] == L' ') spaces++;
        }
        return spaces == hash_len - 1;
    }
};

#endif
