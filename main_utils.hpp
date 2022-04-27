#ifndef MAIN_UTILS
#define MAIN_UTILS

#include <iostream>
#include <fstream>
#include <thread>
#include <atomic>
#include <map>
#include <array>
#include <string>
#include <filesystem>
#include <set>
#include "UTF16LE_file_pars_utils.hpp"
#include "UTF16LEChars.h"

class MainUtils {
	static const dword max_buf_size = 0x2000000;
	byte *main_buf, *backup_buf;
	bool file_with_long_path_found = false;

public:
	static const byte hash_length = 32;
	enum class StartPoint { main, backup };

	const char* FilesWithLongPaths = "Files with long paths.txt";
	const char* NotCopiedFiles = "Not copied files.txt";
	const char* PossiblyCorruptedFiles = "Possibly corrupted files.txt";
	const char* PossiblyDeletedFromMainFiles = "Possibly deleted from main files.txt";

	const std::filesystem::path main_start_point, backup_start_point;

	MainUtils(std::wstring main_start_point_wstr, std::wstring backup_start_point_wstr);
	MainUtils(const MainUtils&) = delete;
	MainUtils& operator=(const MainUtils&) = delete;
	MainUtils(MainUtils &&) = delete;
	MainUtils& operator=(MainUtils &&) = delete;
	virtual ~MainUtils();

	void calc_hash(byte* buf, dword buf_len, std::array<byte, hash_length>& hash);
	void print_progress(size_t total, std::atomic_int64_t& traversed, bool& stop);
	void WritePossiblyCorruptedFiles(std::filesystem::path& main_path, std::filesystem::path& backup_path, std::ofstream& PossiblyCorruptedFilesLog);
	void WriteNotCopiedFiles(std::filesystem::path& from, std::filesystem::path& to, std::wstring reason, std::ofstream& NotCopiedFilesFilesLog);
	void SplitToRelativePathAndFilename (const std::wstring RelativePathAndFilenameString, std::wstring& RelativePath, std::wstring& filename);

	std::set<std::wstring> TraverseFilesToGetNames(StartPoint sp, std::ofstream& LongPathsLog);

	void TraverseSetToForm_size_ext__filename_multimap(StartPoint sp, std::set<std::wstring>& st, std::multimap<std::pair<std::uintmax_t, std::wstring>, std::wstring>& mm);
	void TraverseAndCmpProbablyNotChangedFiles(std::set<std::wstring>& probably_not_changed, std::ofstream& PossiblyCorruptedFilesLog);

	void CmpSingleFilesOnMainAndBackup_andMoveEqualWithinBackup(std::multimap<std::pair<std::uintmax_t, std::wstring>, std::wstring>& size_ext__filename_main_mm,
		std::multimap<std::pair<std::uintmax_t, std::wstring>, std::wstring>& size_ext__filename_backup_mm, std::ofstream& NotCopiedFilesLog);
	void CmpSingleFilesOnMainAndBackup_andMoveEqualWithinBackup(std::multimap<std::array<byte, hash_length>, std::wstring>& hash__filename_main_mm,
		std::multimap<std::array<byte, hash_length>, std::wstring>& hash__filename_backup_mm, std::ofstream& NotCopiedFilesLog);

	void CopyNewUniqueFilesFromMainIntoBackup(std::multimap<std::pair<std::uintmax_t, std::wstring>, std::wstring>& size_ext__filename_main_mm, std::ofstream& NotCopiedFilesLog);
	void CopyFilesFromMainIntoBackup(std::multimap<std::array<byte, hash_length>, std::wstring>& hash__filename_main_mm, 
		std::multimap<std::array<byte, hash_length>, std::wstring>& hash__filename_backup_mm,
		std::ofstream& NotCopiedFilesLog);
	
	void WriteToFilePossiblyDeletedFilesFromMain(std::multimap<std::array<byte, hash_length>, std::wstring>& hash__filename_backup_mm, std::ofstream& PossiblyDeletedFromMainFilesLog);

	bool FileWithLongPathHasBeenFound() { return file_with_long_path_found; }

	void Traverse_size_ext__filename_mm_ToForm_hash__filename_mm(StartPoint sp, std::multimap<std::pair<std::uintmax_t, std::wstring>, std::wstring>&  size_ext__filename_mm, 
		std::multimap<std::array<byte, hash_length>, std::wstring>& hash__filename_mm);
};

MainUtils::MainUtils(std::wstring main_start_point_wstr, std::wstring backup_start_point_wstr) : main_start_point(main_start_point_wstr), 
backup_start_point(backup_start_point_wstr) {
	main_buf = new byte[max_buf_size];
	backup_buf = new byte[max_buf_size];
}

MainUtils::~MainUtils() {
	delete [] main_buf;
	delete [] backup_buf;
}

void MainUtils::calc_hash(byte* buf, dword buf_len, std::array<byte, MainUtils::hash_length>& hash) {
	dword buf_offs = 0;
	for (; buf_offs + hash_length < buf_len; buf_offs += MainUtils::hash_length) {
		for (byte i = 0; i < MainUtils::hash_length; i++)
			hash[i] ^= buf[buf_offs + i];
	}

	for (byte i = 0; buf_offs + i < buf_len; i++)
		hash[i] ^= buf[buf_offs + i];
}

void MainUtils::print_progress(size_t total, std::atomic_int64_t& traversed, bool& stop) {
	if (total == 0) {
		while (!stop) {
			std::cout << "progress: " << traversed << " entries\r";
			std::this_thread::sleep_for(std::chrono::seconds(2));
		}
	}
	else {
		while (!stop) {
			double progress = (double)traversed / (double)total * 100.0;
			std::cout << "progress: " << progress << "%          \r";
			std::this_thread::sleep_for(std::chrono::seconds(2));
		}
	}
	std::cout << "done                             " << std::endl << std::endl;
}

void MainUtils::WritePossiblyCorruptedFiles(std::filesystem::path& main_path, std::filesystem::path& backup_path, std::ofstream& PossiblyCorruptedFilesLog) {
	PossiblyCorruptedFilesLog.write((char*)(main_path.wstring().data()), main_path.wstring().size() * 2);
	PossiblyCorruptedFilesLog.write((char*)&UTF16LE::NewLine, 2);
	PossiblyCorruptedFilesLog.write((char*)(backup_path.wstring().data()), backup_path.wstring().size() * 2);
	PossiblyCorruptedFilesLog.write((char*)&UTF16LE::NewLine, 2);
	PossiblyCorruptedFilesLog.write((char*)&UTF16LE::NewLine, 2);
}

void MainUtils::WriteNotCopiedFiles(std::filesystem::path& from, std::filesystem::path& to, std::wstring reason, std::ofstream& NotCopiedFilesFilesLog) {
	NotCopiedFilesFilesLog.write((char*)(from.wstring().data()), from.wstring().size() * 2);
	NotCopiedFilesFilesLog.write((char*)&UTF16LE::NewLine, 2);
	NotCopiedFilesFilesLog.write((char*)(to.wstring().data()), to.wstring().size() * 2);
	NotCopiedFilesFilesLog.write((char*)&UTF16LE::NewLine, 2);
	NotCopiedFilesFilesLog.write((char*)(reason.data()), reason.size() * 2);
	NotCopiedFilesFilesLog.write((char*)&UTF16LE::NewLine, 2);
	NotCopiedFilesFilesLog.write((char*)&UTF16LE::NewLine, 2);
}

std::set<std::wstring> MainUtils::TraverseFilesToGetNames(StartPoint sp, std::ofstream& LongPathsLog) {
	std::set<std::wstring> files;
	const std::filesystem::path* start_point = nullptr;

	switch (sp) {
	case StartPoint::main:
		start_point = &main_start_point;
		break;
	case StartPoint::backup:
		start_point = &backup_start_point;
		break;
	}

	if (start_point == nullptr) return files;

	std::filesystem::recursive_directory_iterator rec_it(*start_point); //recursive iterator

	std::atomic_int64_t traversed = 0;
	bool stop_ppt = false;

	std::thread ppt(&MainUtils::print_progress, this, 0, std::ref(traversed), std::ref(stop_ppt)); //print progress thread
	for (const std::filesystem::directory_entry& e : rec_it) {
		if (e.is_regular_file()) {
			if (e.path().wstring().size() >= 256) {
				file_with_long_path_found = true;
				LongPathsLog.write((char*)e.path().wstring().data(), e.path().wstring().size() * 2);
				LongPathsLog.write((char*)&UTF16LE::NewLine, 2);
			}
			std::wstring full_path(e.path().parent_path().wstring());
			std::wstring relative_path(full_path.replace(0, (*start_point).wstring().length(), L""));
			std::wstring filename(e.path().filename().wstring());
			std::wstring full_filename(relative_path);
			if (full_filename != L"") full_filename += L"/";
			full_filename += filename;
			files.emplace(full_filename);
		}
		traversed++;
	}
	stop_ppt = true;
	ppt.join();
	
	return files;
}

void MainUtils::TraverseSetToForm_size_ext__filename_multimap(StartPoint sp, std::set<std::wstring>& st, std::multimap<std::pair<std::uintmax_t, std::wstring>, std::wstring>& mm) {
	const std::filesystem::path* start_point = nullptr;
	switch (sp) {
	case StartPoint::main:
		start_point = &main_start_point;
		break;
	case StartPoint::backup:
		start_point = &backup_start_point;
		break;
	}
	if (start_point == nullptr) return;

	for (const std::wstring & filename : st) {
		std::filesystem::path path( (*start_point) / filename);
		std::uintmax_t file_size = std::filesystem::file_size(path);
		mm.emplace(std::pair<std::uintmax_t, std::wstring>(file_size, path.extension()), filename);
	}
}

void MainUtils::TraverseAndCmpProbablyNotChangedFiles(std::set<std::wstring>& probably_not_changed, std::ofstream& PossiblyCorruptedFilesLog) {
	if (probably_not_changed.size() == 0) {
		std::cout << "not required" << std::endl << std::endl;
		return;
	}
	std::atomic_int64_t traversed = 0;
	bool stop_ppt = false;
	std::thread ppt(&MainUtils::print_progress, this, probably_not_changed.size(), std::ref(traversed), std::ref(stop_ppt)); //print progress thread
	for (const std::wstring & filename : probably_not_changed) {
		std::filesystem::path main_path(main_start_point / filename);
		std::filesystem::path backup_path(backup_start_point / filename);
		std::uintmax_t main_file_size = std::filesystem::file_size(main_path);
		std::uintmax_t backup_file_size = std::filesystem::file_size(backup_path);
		
		if (main_file_size != backup_file_size) {
			WritePossiblyCorruptedFiles(main_path, backup_path, PossiblyCorruptedFilesLog);
			continue;
		}

		std::ifstream main_in(main_path, std::ios::binary);
		std::ifstream backup_in(backup_path, std::ios::binary);
		#define CLOSE_FILES main_in.close(); backup_in.close();

		dword buf_size = (main_file_size < max_buf_size) ? main_file_size : max_buf_size;
		//note, we've checked that main_file_size == backup_file_size, so it doesn't matter which of two variables will be used

		std::uintmax_t offs = 0;
		for (; offs + buf_size < main_file_size; offs += buf_size) {
			main_in.read((char*)main_buf, buf_size);
			backup_in.read((char*)backup_buf, buf_size);
			if (memcmp(main_buf, backup_buf, buf_size) != 0) {
				WritePossiblyCorruptedFiles(main_path, backup_path, PossiblyCorruptedFilesLog);
				CLOSE_FILES
				continue;
			}
		}

		main_in.read((char*)main_buf, main_file_size - offs);
		backup_in.read((char*)backup_buf, main_file_size - offs);
		if (memcmp(main_buf, backup_buf, buf_size) != 0) {
			WritePossiblyCorruptedFiles(main_path, backup_path, PossiblyCorruptedFilesLog);
			CLOSE_FILES
			continue;
		}
		
		CLOSE_FILES
		#undef CLOSE_FILES
		traversed++;
	}

	stop_ppt = true;
	ppt.join();
}

void MainUtils::SplitToRelativePathAndFilename(const std::wstring RelativePathAndFilenameString, std::wstring& RelativePath, std::wstring& filename) {
	int last_slash = -1;
	for (int i = 0; i < RelativePathAndFilenameString.size(); i++) {
		if (RelativePathAndFilenameString[i] == '\\' || RelativePathAndFilenameString[i] == '/')
			last_slash = i;
	}
	if (last_slash == -1) {
		filename = RelativePathAndFilenameString;
		return;
	}

	RelativePath = RelativePathAndFilenameString.substr(0, last_slash);
	filename = RelativePathAndFilenameString.substr(last_slash + 1);
}

void MainUtils::Traverse_size_ext__filename_mm_ToForm_hash__filename_mm(StartPoint sp, std::multimap<std::pair<std::uintmax_t, std::wstring>, std::wstring>& size_ext__filename_mm,
	std::multimap<std::array<byte, hash_length>, std::wstring>& hash__filename_mm) {
	if (size_ext__filename_mm.size() == 0) {
		std::cout << "not required" << std::endl << std::endl;
		return;
	}
	const std::filesystem::path* start_point = nullptr;

	switch (sp) {
	case StartPoint::main:
		start_point = &main_start_point;
		break;
	case StartPoint::backup:
		start_point = &backup_start_point;
		break;
	}
	if (start_point == nullptr) return;

	std::atomic_int64_t traversed = 0;
	bool stop_ppt = false;
	std::thread ppt(&MainUtils::print_progress, this, size_ext__filename_mm.size(), std::ref(traversed), std::ref(stop_ppt)); //print progress thread

	for (std::pair<const std::pair<std::uintmax_t, std::wstring>, std::wstring>& it : size_ext__filename_mm) {
		std::filesystem::path path((*start_point) / it.second);
		std::uintmax_t file_size = std::filesystem::file_size(path);
		std::ifstream in(path, std::ios::binary);
		dword buf_size = (file_size < max_buf_size) ? file_size : max_buf_size;

		std::array<byte, hash_length> hash;
		for (byte& b : hash) b = 0;

		std::uintmax_t offs = 0;
		for (; offs + buf_size < file_size; offs += buf_size) {
			in.read((char*)main_buf, buf_size); //here it doesn't matter which of two buffers to use - main_buf or backup_buf, we will use main_buf
			calc_hash(main_buf, buf_size, hash);
		}
		in.read((char*)main_buf, file_size - offs);
		calc_hash(main_buf, file_size - offs, hash);

		hash__filename_mm.emplace(hash, it.second);
		in.close();
		traversed++;
	}
	stop_ppt = true;
	ppt.join();
}

void MainUtils::CmpSingleFilesOnMainAndBackup_andMoveEqualWithinBackup(
	std::multimap<std::pair<std::uintmax_t, std::wstring>, std::wstring>& size_ext__filename_main_mm,
	std::multimap<std::pair<std::uintmax_t, std::wstring>, std::wstring>& size_ext__filename_backup_mm, 
	std::ofstream& NotCopiedFilesLog) { //both multimaps contain the pairs <size, extension> as keys, and relative path/filenames as values

	if (size_ext__filename_main_mm.size() == 0 || size_ext__filename_backup_mm.size() == 0) {
		std::cout << "not required" << std::endl << std::endl;
		return;
	}
	std::list<std::pair<std::uintmax_t, std::wstring>> KeysToDelete;

	std::atomic_int64_t traversed = 0;
	bool stop_ppt = false;
	std::thread ppt(&MainUtils::print_progress, this, size_ext__filename_main_mm.size(), std::ref(traversed), std::ref(stop_ppt)); //print progress thread

	for (std::pair<const std::pair<std::uintmax_t, std::wstring>, std::wstring>& it : size_ext__filename_main_mm) {
		if (size_ext__filename_main_mm.count(it.first) == 1) {
			if (size_ext__filename_backup_mm.count(it.first) == 1) {
				//compare files on main and backup
				std::filesystem::path main_path(main_start_point / it.second);
				auto it2 = size_ext__filename_backup_mm.find(it.first);
				std::filesystem::path backup_path(backup_start_point / it2->second);

				std::ifstream main_in(main_path, std::ios::binary);
				std::ifstream backup_in(backup_path, std::ios::binary);
				#define CLOSE_FILES main_in.close(); backup_in.close();

				const std::uintmax_t& main_file_size = it.first.first;

				dword buf_size = (main_file_size < max_buf_size) ? main_file_size : max_buf_size;
				
				std::uintmax_t offs = 0;
				for (; offs + buf_size < main_file_size; offs += buf_size) {
					main_in.read((char*)main_buf, buf_size);
					backup_in.read((char*)backup_buf, buf_size);
					if (memcmp(main_buf, backup_buf, buf_size) != 0) {
						CLOSE_FILES
						continue;
					}
				}
				
				main_in.read((char*)main_buf, main_file_size - offs);
				backup_in.read((char*)backup_buf, main_file_size - offs);
				if (memcmp(main_buf, backup_buf, buf_size) != 0) {
					CLOSE_FILES
					continue;
				}

				CLOSE_FILES 
				#undef CLOSE_FILES

				std::wstring MainRelativePath, MainFilename, BackupRelativePath, BackupFilename;
				SplitToRelativePathAndFilename(it.second, MainRelativePath, MainFilename);
				SplitToRelativePathAndFilename(it2->second, BackupRelativePath, BackupFilename);

				std::filesystem::path new_bckup_path(backup_start_point / MainRelativePath);
				if (MainRelativePath != BackupRelativePath) {
					try { std::filesystem::create_directories(new_bckup_path); }
					catch (const std::filesystem::filesystem_error& e) {
						WriteNotCopiedFiles(backup_path, new_bckup_path, L"cannot create directories", NotCopiedFilesLog);
						KeysToDelete.push_back(it.first);
						continue;
					}
				}

				new_bckup_path /= MainFilename;

				//to summarize. here must be std::filesystem::rename(backup start point/relative path/filename, backup start point/main relative path/ main filename);
				try { std::filesystem::rename(backup_path, new_bckup_path); }
				catch (const std::filesystem::filesystem_error& e) {
					WriteNotCopiedFiles(backup_path, new_bckup_path, L"cannot rename file", NotCopiedFilesLog);
					KeysToDelete.push_back(it.first);
					continue;
				}

				KeysToDelete.push_back(it.first);
			}
		}
		traversed++;
	}

	stop_ppt = true;
	ppt.join();

	for (std::pair<std::uintmax_t, std::wstring> & key : KeysToDelete) {
		size_ext__filename_main_mm.erase(key);
		size_ext__filename_backup_mm.erase(key);
	}
}

void MainUtils::CmpSingleFilesOnMainAndBackup_andMoveEqualWithinBackup(std::multimap<std::array<byte, hash_length>, std::wstring>& hash__filename_main_mm,
	std::multimap<std::array<byte, hash_length>, std::wstring>& hash__filename_backup_mm, std::ofstream& NotCopiedFilesLog) {
	//both multimaps contain the hashes as keys, and relative path/filenames as values
	if (hash__filename_main_mm.size() == 0 || hash__filename_backup_mm.size() == 0) {
		std::cout << "not required" << std::endl << std::endl;
		return;
	}
	std::list<std::array<byte, hash_length>> KeysToDelete;

	std::atomic_int64_t traversed = 0;
	bool stop_ppt = false;
	std::thread ppt(&MainUtils::print_progress, this, hash__filename_main_mm.size(), std::ref(traversed), std::ref(stop_ppt)); //print progress thread

	for (std::pair<const std::array<byte, hash_length>, std::wstring>& it : hash__filename_main_mm) {
		if (hash__filename_main_mm.count(it.first) == 1) {
			if (hash__filename_backup_mm.count(it.first) == 1) {
				std::filesystem::path main_path(main_start_point / it.second);
				auto it2 = hash__filename_backup_mm.find(it.first);
				std::filesystem::path old_backup_path(backup_start_point / it2->second);

				std::wstring MainRelativePath, MainFilename, BackupRelativePath, BackupFilename;
				SplitToRelativePathAndFilename(it.second, MainRelativePath, MainFilename);
				SplitToRelativePathAndFilename(it2->second, BackupRelativePath, BackupFilename);

				std::filesystem::path new_bckup_path(backup_start_point / MainRelativePath);
				if (MainRelativePath != BackupRelativePath) {
					try { std::filesystem::create_directories(new_bckup_path); }
					catch (const std::filesystem::filesystem_error& e) {
						WriteNotCopiedFiles(old_backup_path, new_bckup_path, L"cannot create directories", NotCopiedFilesLog);
					}
				}

				new_bckup_path /= MainFilename;

				//to summarize. here must be std::filesystem::rename(backup start point/relative path/filename , backup start point/main relative path/ main filename);
				try { std::filesystem::rename(old_backup_path, new_bckup_path); }
				catch (const std::filesystem::filesystem_error& e) {
					WriteNotCopiedFiles(old_backup_path, new_bckup_path, L"cannot move file", NotCopiedFilesLog);
				}

				KeysToDelete.push_back(it.first);
			}
		}
		traversed++;
	}

	stop_ppt = true;
	ppt.join();

	for (std::array<byte, hash_length>& key : KeysToDelete) {
		hash__filename_main_mm.erase(key);
		hash__filename_backup_mm.erase(key);
	}
}

void MainUtils::CopyNewUniqueFilesFromMainIntoBackup(std::multimap<std::pair<std::uintmax_t, std::wstring>, std::wstring>& size_ext__filename_main_mm, std::ofstream& NotCopiedFilesLog) {
	if (size_ext__filename_main_mm.size() == 0) {
		std::cout << "not required" << std::endl << std::endl;
		return;
	}
	std::list<std::pair<std::uintmax_t, std::wstring>> KeysToDelete;

	std::atomic_int64_t traversed = 0;
	bool stop_ppt = false;
	std::thread ppt(&MainUtils::print_progress, this, size_ext__filename_main_mm.size(), std::ref(traversed), std::ref(stop_ppt)); //print progress thread

	for (std::pair<const std::pair<std::uintmax_t, std::wstring>, std::wstring>& it : size_ext__filename_main_mm) {
		if (size_ext__filename_main_mm.count(it.first) == 1) {
			std::wstring MainRelativePath, MainFilename, BackupRelativePath, BackupFilename;
			SplitToRelativePathAndFilename(it.second, MainRelativePath, MainFilename);
			std::filesystem::path new_bckup_path(backup_start_point / MainRelativePath);
			std::filesystem::path main_path(main_start_point / it.second);
			
			if (new_bckup_path.wstring().size() >= 256) { //if new path is too long, copying is not possible
				WriteNotCopiedFiles(main_path, new_bckup_path, L"new path is more than 256 characters", NotCopiedFilesLog);
				continue;
			}

			if (!std::filesystem::exists(new_bckup_path)) {
				try { std::filesystem::create_directories(new_bckup_path); }
				catch (const std::filesystem::filesystem_error& e) {
					WriteNotCopiedFiles(main_path, new_bckup_path, L"cannot create directories", NotCopiedFilesLog);
					continue;
				}
			}

			new_bckup_path /= MainFilename;

			try { std::filesystem::copy_file(main_path, new_bckup_path); }
			catch (const std::filesystem::filesystem_error& e) { 
				WriteNotCopiedFiles(main_path, new_bckup_path, L"cannot copy file", NotCopiedFilesLog);
			}

			KeysToDelete.push_back(it.first);
		}
		traversed++;
	}
	stop_ppt = true;
	ppt.join();

	for (std::pair<std::uintmax_t, std::wstring>& key : KeysToDelete)
		size_ext__filename_main_mm.erase(key);
}

void MainUtils::CopyFilesFromMainIntoBackup(std::multimap<std::array<byte, hash_length>, std::wstring>& hash__filename_main_mm, 
	std::multimap<std::array<byte, hash_length>, std::wstring>& hash__filename_backup_mm, std::ofstream& NotCopiedFilesLog) {
	if (hash__filename_main_mm.size() == 0) {
		std::cout << "not required" << std::endl << std::endl;
		return;
	}
	
	std::atomic_int64_t traversed = 0;
	bool stop_ppt = false;
	std::thread ppt(&MainUtils::print_progress, this, hash__filename_main_mm.size(), std::ref(traversed), std::ref(stop_ppt)); //print progress thread

	for (std::pair<const std::array<byte, hash_length>, std::wstring>& it : hash__filename_main_mm) {
		auto it2 = hash__filename_backup_mm.find(it.first);
		if (it2 != hash__filename_backup_mm.end()) {
			std::filesystem::path main_path(main_start_point / it.second);
			std::filesystem::path backup_path(backup_start_point / it2->second);
			std::uintmax_t main_file_size = std::filesystem::file_size(main_path);
			std::uintmax_t backup_file_size = std::filesystem::file_size(backup_path);

			if (main_file_size != backup_file_size)
				goto NotTheSameFileBranch;
			
			std::wstring MainRelativePath, MainFilename, BackupRelativePath, BackupFilename;
			SplitToRelativePathAndFilename(it.second, MainRelativePath, MainFilename);
			SplitToRelativePathAndFilename(it2->second, BackupRelativePath, BackupFilename);
			
			std::filesystem::path new_bckup_path(backup_start_point / MainRelativePath);
			if (MainRelativePath != BackupRelativePath) {
				try { std::filesystem::create_directories(new_bckup_path); }
				catch (const std::filesystem::filesystem_error& e) {
					WriteNotCopiedFiles(backup_path, new_bckup_path, L"cannot create directories", NotCopiedFilesLog);
				}
			}

			new_bckup_path /= MainFilename;

			try { std::filesystem::copy_file(backup_path, new_bckup_path); }
			catch (const std::filesystem::filesystem_error& e) {
				WriteNotCopiedFiles(backup_path, new_bckup_path, L"cannot copy file", NotCopiedFilesLog);
			}
		}
		else {
		NotTheSameFileBranch:
			std::wstring MainRelativePath, MainFilename;
			SplitToRelativePathAndFilename(it.second, MainRelativePath, MainFilename);
			std::filesystem::path new_bckup_path(backup_start_point / MainRelativePath);
			std::filesystem::path main_path(main_start_point / it.second);

			if (new_bckup_path.wstring().size() >= 256) { //if new path is too long, copying is not possible
				WriteNotCopiedFiles(main_path, new_bckup_path, L"new path is more than 256 characters", NotCopiedFilesLog);
				continue;
			}

			if (!std::filesystem::exists(new_bckup_path)) {
				try { std::filesystem::create_directories(new_bckup_path); }
				catch (const std::filesystem::filesystem_error& e) {
					WriteNotCopiedFiles(main_path, new_bckup_path, L"cannot create directories", NotCopiedFilesLog);
					continue;
				}
			}

			new_bckup_path /= MainFilename;

			try { std::filesystem::copy_file(main_path, new_bckup_path); }
			catch (const std::filesystem::filesystem_error& e) {
				WriteNotCopiedFiles(main_path, new_bckup_path, L"cannot copy file", NotCopiedFilesLog);
			}
		}

		traversed++;
	}
	stop_ppt = true;
	ppt.join();
}

void MainUtils::WriteToFilePossiblyDeletedFilesFromMain(std::multimap<std::array<byte, hash_length>, std::wstring>& hash__filename_backup_mm, std::ofstream& PossiblyDeletedFromMainFilesLog) {
	if (hash__filename_backup_mm.size() == 0) return;
	for (std::pair<const std::array<byte, hash_length>, std::wstring>& it : hash__filename_backup_mm) {
		PossiblyDeletedFromMainFilesLog.write((char*)it.second.data(), it.second.size() * 2);
		PossiblyDeletedFromMainFilesLog.write((char*)&UTF16LE::NewLine, 2);
	}
}

#endif