#include <iostream>
#include <fstream>
#include <filesystem>
#include <thread>
#include <chrono>
#include <array>
#include <map>
#include <list>
#include <set>
#include "typedefs.h"
#include "mathset.hpp"
#include "main_utils.hpp"

enum class modes{fast, thorough};

void PreparePossiblyDeletedFromMainFilesLog(std::ofstream & PossiblyDeletedFromMainFilesLog, std::wstring & backup_start_point_wstr) {
	PossiblyDeletedFromMainFilesLog.write((char*)&UTF16LE::signature, 2);
	PossiblyDeletedFromMainFilesLog.write((char*)backup_start_point_wstr.data(), backup_start_point_wstr.size() * 2);
	PossiblyDeletedFromMainFilesLog.write((char*)&UTF16LE::NewLine, 2); PossiblyDeletedFromMainFilesLog.write((char*)&UTF16LE::NewLine, 2);
}

int main(int argc, char* argv[]) {
	if (argc != 2) {
		std::cout << "Use: SynchronizeFiles <config file>" << std::endl;
		std::cout << "The config file must contain the following arguments, each per line, in the fixed order: " << std::endl;
		std::cout << "main start point" << std::endl;
		std::cout << "backup start point" << std::endl;
		std::cout << "mode f|t (fast, thorough)" << std::endl;
		exit(1);
	}
	std::list<std::wstring> args;
	UTF16LEFileParsUtils::GetWstringListFromUTF16LEFile(argv[1], args);
	
	if (args.size() != 3) {
		std::cout << "cannot get arguments from config file" << std::endl;
		exit(1);
	}

	const byte hash_len = 20;
	const std::uintmax_t max_file_size_in_fast_mode = 0x100000;

	std::wstring main_start_point_wstr(args.front());
	args.pop_front();
	if (main_start_point_wstr[main_start_point_wstr.size() - 1] != L'\\' && main_start_point_wstr[main_start_point_wstr.size() - 1] != L'/') main_start_point_wstr += L"/";
	if (!std::filesystem::exists(main_start_point_wstr)) {
		std::cout << "main start point doesn't exists" << std::endl;
		exit(1);
	}
	
	std::wstring backup_start_point_wstr(args.front());
	args.pop_front();
	if (backup_start_point_wstr[backup_start_point_wstr.size() - 1] != L'\\' && backup_start_point_wstr[backup_start_point_wstr.size() - 1] != L'/') backup_start_point_wstr += L"/";
	if (!std::filesystem::exists(backup_start_point_wstr)) {
		std::cout << "backup start point doesn't exists" << std::endl;
		exit(1);
	}

	MainUtils mu(main_start_point_wstr, backup_start_point_wstr);
	
	std::wstring mode_wstr(args.front());
	args.pop_front();
	
	modes mode;
	if (mode_wstr == L"f")
		mode = modes::fast;
	else if (mode_wstr == L"t")
		mode = modes::thorough;
	else {
		std::cout << "cannot recognize mode. mode must be either f or t" << std::endl;
		exit(1);
	}

	std::ofstream LongPathsLog(mu.FilesWithLongPaths, std::ios::binary);
	LongPathsLog.write((char*)&UTF16LE::signature, 2);

	std::cout << "traversing files from main start point" << std::endl;
	std::set<std::wstring> main_filenames(mu.TraverseFilesToGetNames(MainUtils::StartPoint::main, LongPathsLog));

	std::cout << "traversing files from backup start point" << std::endl;
	std::set<std::wstring> backup_filenames(mu.TraverseFilesToGetNames(MainUtils::StartPoint::backup, LongPathsLog));
	LongPathsLog.close();

	if (mu.FileWithLongPathHasBeenFound()) {
		std::cout << "Error: files with very long paths found. See the generated file " << mu.FilesWithLongPaths << " for the file list." << std::endl;
		std::cout << "It's not possible to syncronize directories if they hold files that cannot be moved or copied." << std::endl;
		std::cout << "Please reduce the filenames or paths to them up to 260 chars." << std::endl;
		exit(1);
	}
	
	std::set<std::wstring> probably_not_changed(mathset<std::wstring>::intersection(main_filenames, backup_filenames));
	std::set<std::wstring> probably_new_on_main(mathset<std::wstring>::difference(main_filenames, backup_filenames));
	std::set<std::wstring> probably_new_on_backup(mathset<std::wstring>::difference(backup_filenames, main_filenames));
	main_filenames.clear();
	backup_filenames.clear();

	std::ofstream PossiblyCorruptedFilesLog(mu.PossiblyCorruptedFiles, std::ios::binary);
	PossiblyCorruptedFilesLog.write((char*)&UTF16LE::signature, 2);

	if (mode == modes::thorough) {
		std::cout << "traversing and comparing probably not changed files" << std::endl;
		mu.TraverseAndCmpProbablyNotChangedFiles(probably_not_changed, PossiblyCorruptedFilesLog); //checking the files on the same relative locations on main and backup
	}
	probably_not_changed.clear();

	//looking for renamed or relocated files to prevent unnecessary copying from main into backup
	//it's quite simple to look at the size of files
	//so we traverse all files in probably_new_on_main and probably_new_on_backup and form 2 multimaps of type <size, extension> -> <filename>
	std::multimap<std::pair<std::uintmax_t, std::wstring>, std::wstring> size_ext__filename_main_mm; //multimap <size, extension> -> <filename> for files in main
	std::multimap<std::pair<std::uintmax_t, std::wstring>, std::wstring> size_ext__filename_backup_mm; //multimap <size, extension> -> <filename> for files in backup
	mu.TraverseSetToForm_size_ext__filename_multimap(MainUtils::StartPoint::main, probably_new_on_main, size_ext__filename_main_mm);
	probably_new_on_main.clear();
	mu.TraverseSetToForm_size_ext__filename_multimap(MainUtils::StartPoint::backup, probably_new_on_backup, size_ext__filename_backup_mm);
	probably_new_on_backup.clear();
	
	std::ofstream NotCopiedFilesLog(mu.NotCopiedFiles, std::ios::binary);
	NotCopiedFilesLog.write((char*)&UTF16LE::signature, 2);
	
	std::cout << "comparing single files in both start points and moving equal within backup" << std::endl;
	mu.CmpSingleFilesOnMainAndBackup_andMoveEqualWithinBackup(size_ext__filename_main_mm, size_ext__filename_backup_mm, NotCopiedFilesLog);

	//now in both multimaps must remain elements like 
	//size, extension -> file1, but there is no same file1 in another storage
	//size, extension -> file2, file 3; some of these files may be in another storage, but at another relative path or under another name

	std::cout << "copying unique single files from main into backup" << std::endl;
	mu.CopyNewUniqueFilesFromMainIntoBackup(size_ext__filename_main_mm, NotCopiedFilesLog);
	//now in size_ext__filename_main_mm must remain elements like size, extension -> file2, file 3
	//in backup must remain elements like 
	//size, extension -> file1,
	//size, extension -> file2, file 3

	std::multimap<std::array<byte, MainUtils::hash_length>, std::wstring> hash__filename_main_mm; //multimap <hash> -> <filename> for files in main
	std::multimap<std::array<byte, MainUtils::hash_length>, std::wstring> hash__filename_backup_mm; //multimap <hash> -> <filename> for files in backup
	std::cout << "calculating hashes for non-unique files in main" << std::endl;
	mu.Traverse_size_ext__filename_mm_ToForm_hash__filename_mm(MainUtils::StartPoint::main, size_ext__filename_main_mm, hash__filename_main_mm);
	size_ext__filename_main_mm.clear();
	std::cout << "calculating hashes for non-unique files in backup" << std::endl;
	mu.Traverse_size_ext__filename_mm_ToForm_hash__filename_mm(MainUtils::StartPoint::backup, size_ext__filename_backup_mm, hash__filename_backup_mm);
	size_ext__filename_backup_mm.clear();
	//now in both multimaps must be elements like 
	//hash1 -> file1, and file with this hash may be in another storage, under different name or relative location
	//hash2 -> file2, file3; files with the same hash may be in another storage
	std::cout << "comparing single files in both start points and moving equal within backup" << std::endl;
	mu.CmpSingleFilesOnMainAndBackup_andMoveEqualWithinBackup(hash__filename_main_mm, hash__filename_backup_mm, NotCopiedFilesLog);
	//now in both multimaps must be elements like
	//hash1 -> file1, but there is no file with the same hash in another storage
	//hash2 -> file2, file3; files with the same hash may be in another storage under different name or relative location
	std::cout << "copying files from main into backup" << std::endl;
	mu.CopyFilesFromMainIntoBackup(hash__filename_main_mm, hash__filename_backup_mm, NotCopiedFilesLog);

	std::ofstream PossiblyDeletedFromMainFilesLog(mu.PossiblyDeletedFromMainFiles, std::ios::binary);
	PreparePossiblyDeletedFromMainFilesLog(PossiblyDeletedFromMainFilesLog, backup_start_point_wstr);
	mu.WriteToFilePossiblyDeletedFilesFromMain(hash__filename_backup_mm, PossiblyDeletedFromMainFilesLog);
	
	hash__filename_main_mm.clear();
	hash__filename_backup_mm.clear();

	PossiblyCorruptedFilesLog.close();
	NotCopiedFilesLog.close();
	PossiblyDeletedFromMainFilesLog.close();

	uintmax_t size = std::filesystem::file_size(mu.PossiblyCorruptedFiles);
	if (size == 2) std::filesystem::remove(mu.PossiblyCorruptedFiles);

	size = std::filesystem::file_size(mu.PossiblyDeletedFromMainFiles);
	if (size == mu.backup_start_point.wstring().size() * 2  + 6) std::filesystem::remove(mu.PossiblyDeletedFromMainFiles);

	size = std::filesystem::file_size(mu.NotCopiedFiles);
	if (size == 2) std::filesystem::remove(mu.NotCopiedFiles);

	size = std::filesystem::file_size(mu.FilesWithLongPaths);
	if (size == 2) std::filesystem::remove(mu.FilesWithLongPaths);

	return 0;
}