#ifndef UTF16LE_FILE_PARS_UTILS_HPP
#define UTF16LE_FILE_PARS_UTILS_HPP

#include <iostream>
#include "UTF16LEChars.h"
#include "parsbufutils.hpp"

class UTF16LEFileParsUtils {
	static void GetWstringListFromUTF16LEFile_lower_level(const std::wstring& filename, std::list<std::wstring>& wstring_list) {
		std::filesystem::path filename_path(filename);

		if (!std::filesystem::is_regular_file(filename_path))
			filename_path = std::filesystem::current_path() / filename_path;

		if (!std::filesystem::is_regular_file(filename_path)) {
			std::cout << "cannot find UTF16LE file" << std::endl;
			exit(1);
		}

		std::uintmax_t file_size = std::filesystem::file_size(filename_path);

		std::ifstream in(filename_path.wstring(), std::ios::binary);
		if (!in.is_open()) {
			std::cout << "cannot open file" << std::endl;
			exit(1);
		}

		wchar_t UTF16LEsignatureFromInput = in.get();
		UTF16LEsignatureFromInput |= in.get() << 8;
		if (UTF16LEsignatureFromInput != UTF16LE::signature) {
			std::cout << "input file isn't UTF16LE file" << std::endl;
			exit(1);
		}
		//note, that because two characters have been read, only file_size - 2 characters left to read from file
		//it's better if the buffer will end with 0x0000, so the buffer of size (file_size - 2 + 2) bytes must be allocated
		qword buf_size = file_size;
		byte* buf = new byte[buf_size];
		in.read((char*)buf, buf_size);
		*((wchar_t*)(buf + buf_size - 2)) = 0x0000;
		wstring_list = ParsBufUtils::BreakBufferIntoWstrings(buf, buf_size);
		in.close();
		delete[] buf;
	}
public:
	static void GetWstringListFromUTF16LEFile(char* filename_as_byte_str, std::list<std::wstring>& wstring_list) {
		const dword filename_as_word_str_len = 1000;
		wchar_t filename_as_word_str[filename_as_word_str_len];
		memset(filename_as_word_str, 0, filename_as_word_str_len);
		ConvUtils::CP1251_to_UTF16LE((byte*)filename_as_byte_str, filename_as_word_str);
		std::wstring filename(filename_as_word_str);
		GetWstringListFromUTF16LEFile_lower_level(filename, wstring_list);
	}

	static void GetWstringListFromUTF16LEFile(wchar_t* filename_as_word_str, std::list<std::wstring>& wstring_list) {
		std::wstring filename(filename_as_word_str);
		GetWstringListFromUTF16LEFile_lower_level(filename, wstring_list);
	}

	static void GetWstringListFromUTF16LEFile(const std::string filename_string, std::list<std::wstring>& wstring_list) {
		const dword filename_as_word_str_len = 1000;
		wchar_t filename_as_word_str[filename_as_word_str_len];
		memset(filename_as_word_str, 0, filename_as_word_str_len);
		ConvUtils::CP1251_to_UTF16LE((byte*)filename_string.c_str(), filename_as_word_str);
		std::wstring filename(filename_as_word_str);
		GetWstringListFromUTF16LEFile_lower_level(filename, wstring_list);
	}

	static void GetWstringListFromUTF16LEFile(const std::wstring& filename_wstring, std::list<std::wstring>& wstring_list) {
		GetWstringListFromUTF16LEFile_lower_level(filename_wstring, wstring_list);
	}
};

#endif