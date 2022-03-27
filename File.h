#pragma once
#include <string>
#include <iostream>
#include <fstream>
#include <filesystem>
#include "typedefs.h"
#include <array>

class File {
public:
	static const byte hash_length = 20;

private:
	std::wstring & start_point;
	std::wstring relative_path;
	std::wstring filename;
	std::uintmax_t size;
	std::array<byte, hash_length> hash; //contents hash
public:
	
	File(std::wstring& start_point, std::wstring relative_path, std::wstring filename, std::uintmax_t size, std::array<byte, hash_length> & hash) :
		start_point(start_point), relative_path(relative_path), filename(filename), size(size), hash(hash) {}
	
	File(const File& other) = default;
	File& operator=(const File& other) = default;
	File(File&& other) = default;
	File& operator=(File&& other) = default;
	
	std::array<byte, hash_length> GetHash() const noexcept { return hash; }

	std::wstring GetStartPoint() { return start_point; }
	std::wstring GetRelativePath() { return relative_path; }
	std::wstring GetFilename() { return filename; }
	std::uintmax_t GetSize() { return size; }
	std::array<byte, hash_length> GetHash() { return hash; }

	void printStartPoint() const { std::wcout << start_point << std::endl; }
	void printRelativePath() const { std::wcout << relative_path << std::endl; }
	void printFileName() const { std::wcout << filename << std::endl; }
	void printSize() const { std::wcout << size << std::endl; }
	void printHash() const {
		for (byte i = 0; i < hash_length; i++)
			std::cout << std::hex << (dword) hash[i] << " ";
		std::cout << std::endl;
	}

	bool cmpNames(const File& f) { return filename == f.filename; }
	bool cmpRelativePaths(const File& f) { return relative_path == f.relative_path; };

	bool operator<(const File& f) { return size < f.size; }
	bool operator>(const File& f) { return size > f.size; }
	bool operator==(const File& f) { return (size == f.size) && (hash == f.hash); }

	virtual ~File() = default;
};