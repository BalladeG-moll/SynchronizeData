#include <iostream>
#include <fstream>
#include <filesystem>
#include <locale>
#include <thread>
#include <chrono>
#include <map>
#include <list>
#include "File.h"
#include "typedefs.h"
#include "mathset.hpp"
#include "convutils.hpp"
#include "parsbufutils.hpp"
#include "strsemantics.h"
#include "UTF16LEChars.h"
#include "UTF16LE_file_pars_utils.hpp"

enum class file_id { main, backup };

void print_progress(size_t total, std::atomic_int64_t& traversed, bool& stop) {
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

inline void WriteExceptionMessageAndSetFlag(std::ofstream& log, std::wstring description, std::wstring path, bool& exception_happened) {
	log.write((char*)description.data(), description.size() * 2);
	log.write((char*)&UTF16LE::NewLine, 2);
	log.write((char*)path.data(), path.size() * 2);
	log.write((char*)&UTF16LE::NewLine, 2);
	log.flush();
	exception_happened = true;
}

void first_traversal(std::map<std::array<byte, File::hash_length>, File> & main, 
	std::map<std::array<byte, File::hash_length>, File> & backup, std::ofstream & log, bool &exception_happened) {
	//here we concentrate only on the files that reside on both start points
	std::set<std::array <byte, File::hash_length>> toDelete;
	std::atomic_int64_t traversed = 0;
	bool stop_ppt = false;
	std::thread ppt(print_progress, main.size(), std::ref(traversed), std::ref(stop_ppt)); //print progress thread
	for (auto & it : main) {
		if (backup.contains(it.first)) {
			File& bckupFile = backup.at(it.first); //operator[] doesn't work because it requires default constructor
			std::wstring new_bckup_path_wstr(bckupFile.GetStartPoint());
			if (it.second.GetRelativePath() != L"") new_bckup_path_wstr += L"/" + it.second.GetRelativePath();
			bool toRename = false;
			if (!it.second.cmpRelativePaths(bckupFile)) {
				std::filesystem::path new_bckup_path(new_bckup_path_wstr);
				try { std::filesystem::create_directories(new_bckup_path); }
				catch (const std::filesystem::filesystem_error& e) { 
					WriteExceptionMessageAndSetFlag(log, L"cannot create path", new_bckup_path.wstring(), exception_happened);
				}
				toRename = true;
			}
			if (!it.second.cmpNames(bckupFile))
				toRename = true;
			new_bckup_path_wstr += L"/" + it.second.GetFilename();
			if (toRename) {
				std::wstring old_bckup_path_wstr = bckupFile.GetStartPoint();
				if (bckupFile.GetRelativePath() != L"") old_bckup_path_wstr += L"/" + bckupFile.GetRelativePath();
				old_bckup_path_wstr += L"/" + bckupFile.GetFilename();
				std::filesystem::path old_bckup_path(old_bckup_path_wstr);
				std::filesystem::path new_bckup_path(new_bckup_path_wstr);
				try { std::filesystem::rename(old_bckup_path, new_bckup_path); }
				catch (const std::filesystem::filesystem_error& e) {
					WriteExceptionMessageAndSetFlag(log, L"cannot rename file", old_bckup_path.wstring(), exception_happened);
				}
			}
			toDelete.insert(it.first);
		}
		traversed++;
	}
	stop_ppt = true;
	for (auto& it : toDelete) {
		main.erase(it);
		backup.erase(it);
	}
	ppt.join();
}

void deleteSlashFromEndIfNecessary(wchar_t* str) {
	int i = 0;
	for (; str[i] != 0; i++);
	if (str[i - 1] == L'/' || str[i - 1] == L'\\') {
		str[i - 1] = 0;
	}
}

void fill_hash_to_file_map(std::wstring & filename, std::wstring & start_point_wstr, std::map<std::array<byte, File::hash_length>, File> & m, file_id fid) {
	std::list<std::wstring> wstring_list;
	std::error_code ec;

	switch (fid) {
	case(file_id::main):
		std::cout << "handling main file" << std::endl;
		break;
	case(file_id::backup):
		std::cout << "handling backup file" << std::endl;
		break;
	}
	UTF16LEFileParsUtils::GetWstringListFromUTF16LEFile(filename.data(), wstring_list);

	start_point_wstr = wstring_list.front();
	std::filesystem::path start_point(start_point_wstr);
	if (!std::filesystem::exists(start_point)) {
		std::cout << "start point specified in the metadata file doesn't exist" << std::endl;
		exit(1);
	}
	wstring_list.pop_front();

	std::atomic_int64_t traversed = 0;
	bool stop_thrd = false;
	std::thread thrd(print_progress, wstring_list.size(), std::ref(traversed), std::ref(stop_thrd));

	for (auto it = wstring_list.begin(); it != wstring_list.end(); ) {
		bool err = false;
		std::wstring& filename = *it;
		it++;
		std::wstring relative_path(*it);
		if (relative_path == L"NO PATH") relative_path = L"";
		it++;
		std::wstring& hash_wstr = *it;
		if (!StrSemantics::isHash(hash_wstr, File::hash_length)) err = true;
		it++;
		if (err) {
			std::cout << "error in the metadata file. for details see the generated error file" << std::endl;
			std::ofstream out("error.txt", std::ios::binary);
			out.write((char*)(filename.data()), filename.size() * 2);
			out.write((char*)&UTF16LE::NewLine, 2);
			out.write((char*)(relative_path.data()), relative_path.size() * 2);
			out.write((char*)&UTF16LE::NewLine, 2);
			out.write((char*)(hash_wstr.data()), hash_wstr.size() * 2);
			out.write((char*)&UTF16LE::NewLine, 2);
			out.close();
			exit(1);
		}

		std::wstring full_filename (start_point_wstr);
		if (relative_path != L"") full_filename += L"/" + relative_path;
		full_filename += L"/" + filename;

		std::error_code ec;
		std::uintmax_t file_size = std::filesystem::file_size(std::filesystem::path(full_filename), ec);
		if (ec) {
			std::cout << "cannot find out file size. seems like the path is wrong. for details see the generated error file" << std::endl;
			std::ofstream out("error.txt", std::ios::binary);
			out.write((char*)full_filename.data(), full_filename.size() * 2);
			out.close();
			exit(1);
		}
		
		std::array<byte, File::hash_length> hash;
		ConvUtils::UTF16LE_wstring_to_array<File::hash_length>(hash_wstr, hash);
		File f(start_point_wstr, relative_path, filename, file_size, hash);
		m.emplace(std::pair<std::array<byte, File::hash_length>, File>(f.GetHash(), f));
		traversed++;
	}
	stop_thrd = true;
	thrd.join();
}

int main(int argc, char * argv[]) {	
	if (argc != 2) {
		std::cout << "Use: SynchronizeData <config file>" << std::endl;
		std::cout << "The config file must contain the following arguments, each per line, in the fixed order: " << std::endl;
		std::cout << "main metadata file" << std::endl;
		std::cout << "backup metadata file" << std::endl;
		exit(1);
	}
	std::list<std::wstring> args;
	UTF16LEFileParsUtils::GetWstringListFromUTF16LEFile(argv[1], args);

	if (args.size() != 2) {
		std::cout << "cannot get arguments from config file" << std::endl;
		exit(1);
	}

	std::wstring main_metadata_filename(args.front());
	args.pop_front();
	std::wstring backup_metadata_filename(args.front());
	args.pop_front();
	
	std::map<std::array<byte, File::hash_length>, File> main;
	std::map<std::array<byte, File::hash_length>, File> backup;

	std::wstring main_start_point_wstr, backup_start_point_wstr;

	std::cout << "analyzing main metadata file" << std::endl;
	fill_hash_to_file_map(main_metadata_filename, main_start_point_wstr, main, file_id::main);

	std::cout << "analyzing backup metadata file" << std::endl;
	fill_hash_to_file_map(backup_metadata_filename, backup_start_point_wstr, backup, file_id::backup);

	std::filesystem::path main_start_point(main_start_point_wstr);
	std::filesystem::path backup_start_point(backup_start_point_wstr);

	std::ofstream log("not copied.txt", std::ios::binary);
	bool exception_happened = false;
	log.write((char*)&UTF16LE::signature, 2);

	std::cout << "searching for the same files in main and backup" << std::endl;
	first_traversal(main, backup, log, exception_happened);
	//now in main and backup must be no same files

	std::set<std::pair<std::wstring, std::uintmax_t>> main_files, backup_files;
	std::map< std::pair<std::wstring, std::uintmax_t>, std::array<byte, File::hash_length>> main_file_hash_map, backup_file_hash_map;
	for (auto& it : main) {
		main_file_hash_map[std::pair<std::wstring, std::uintmax_t>(it.second.GetFilename(), it.second.GetSize())] = it.first;
		main_files.insert(std::pair<std::wstring, std::uintmax_t>(it.second.GetFilename(), it.second.GetSize()));
	}

	for (auto& it : backup) {
		backup_file_hash_map[std::pair<std::wstring, std::uintmax_t>(it.second.GetFilename(), it.second.GetSize())] = it.first;
		backup_files.insert(std::pair<std::wstring, std::uintmax_t>(it.second.GetFilename(), it.second.GetSize()));
	}

	std::cout << "copying new files from main to backup" << std::endl;
	
	std::set<std::pair<std::wstring, std::uintmax_t>>supposedly_new_on_main(mathset<std::pair<std::wstring, std::uintmax_t>>::difference(main_files, backup_files));

	std::ofstream out;
	out.open("probably corrupted.txt", std::ios::binary);
	out.write((char*)&UTF16LE::signature, 2);

	std::atomic_int64_t traversed = 0;
	bool stop_ppt = false; //print progress thread

	std::thread ppt1(print_progress, supposedly_new_on_main.size(), std::ref(traversed), std::ref(stop_ppt)); //print progress thread 1

	auto WriteCannotCopyFile = [&log, &exception_happened](std::wstring from, std::wstring to) {
		WriteExceptionMessageAndSetFlag(log, L"cannot copy file from", from, exception_happened);
		WriteExceptionMessageAndSetFlag(log, L"to", to, exception_happened);
	};

	auto WriteProbablyCorruptedFiles = [&out](std::wstring main_path_wstr, std::wstring bckup_path_wstr) {
		out.write((char*)main_path_wstr.data(), main_path_wstr.size() * 2);
		out.write((char*)&UTF16LE::NewLine, 2);
		out.write((char*)bckup_path_wstr.data(), bckup_path_wstr.size() * 2);
		out.write((char*)&UTF16LE::NewLine, 2);
		out.write((char*)&UTF16LE::NewLine, 2);
	};

	for (auto& it : supposedly_new_on_main) { //"it" is a file name
		File& file_in_main{ main.at(main_file_hash_map[it]) };
		std::wstring main_path_wstr(file_in_main.GetStartPoint());

		if (file_in_main.GetRelativePath() != L"") main_path_wstr += L"/" + file_in_main.GetRelativePath();
		main_path_wstr += L"/" + file_in_main.GetFilename();
		std::filesystem::path main_path(main_path_wstr);

		std::wstring new_bckup_path_wstr(backup_start_point_wstr);
		if (file_in_main.GetRelativePath() != L"") new_bckup_path_wstr += L"/" + file_in_main.GetRelativePath();

		std::filesystem::path new_bckup_path(new_bckup_path_wstr);
		try { std::filesystem::create_directories(new_bckup_path); }
		catch (const std::filesystem::filesystem_error & e) {
			WriteExceptionMessageAndSetFlag(log, L"error at creating directory", new_bckup_path_wstr.data(), exception_happened);
		}
		new_bckup_path_wstr += L"/" + file_in_main.GetFilename();

		if (new_bckup_path_wstr.size() >= 256) { //if new path is too long, copying is not possible
			WriteExceptionMessageAndSetFlag(log, L"new backup path is too long", new_bckup_path_wstr.data(), exception_happened);
		}

		if (exception_happened) {
			WriteCannotCopyFile(main_path_wstr.data(), new_bckup_path_wstr.data());
			main_files.erase(it);
			traversed++;
			continue;
		}

		new_bckup_path = std::filesystem::path(new_bckup_path_wstr);

		if (std::filesystem::exists(new_bckup_path)) {
			std::uintmax_t file_size = std::filesystem::file_size(new_bckup_path);
			auto it2 = backup_files.find(std::pair<std::wstring, std::uintmax_t>(file_in_main.GetFilename(), file_size));
			if (it2 != backup_files.end()) {
				WriteProbablyCorruptedFiles(main_path_wstr, new_bckup_path_wstr);
				backup_files.erase(it2);
			}
		}
		else {
			try { std::filesystem::copy_file(main_path, new_bckup_path); }
			catch (const std::filesystem::filesystem_error& e) { WriteCannotCopyFile(main_path_wstr.data(), new_bckup_path_wstr.data()); }
		}

		main_files.erase(it);
		traversed++;
	}
	stop_ppt = true;
	log.close();
	if (!exception_happened) std::remove("not copied.txt");
	ppt1.join();

	traversed = 0;
	stop_ppt = false;
	
	std::cout << "searching for corrupted files" << std::endl;
	std::set<std::pair<std::wstring, std::uintmax_t>> probably_corrupted(mathset<std::pair<std::wstring, std::uintmax_t>>::intersection(main_files, backup_files));

	std::thread ppt2(print_progress, probably_corrupted.size(), std::ref(traversed), std::ref(stop_ppt)); //print progress thread 2
	for (auto& it : probably_corrupted) {
		File& file_in_backup{ backup.at(backup_file_hash_map[it]) };
		std::wstring bckup_path_wstr(file_in_backup.GetStartPoint());
		if (file_in_backup.GetRelativePath() != L"") bckup_path_wstr += L"/" + file_in_backup.GetRelativePath();
		bckup_path_wstr += L"/" + file_in_backup.GetFilename();

		File& file_in_main{ main.at(main_file_hash_map[it]) };
		std::wstring main_path_wstr(file_in_main.GetStartPoint());
		if (file_in_main.GetRelativePath() != L"") main_path_wstr += L"/" + file_in_main.GetRelativePath();
		main_path_wstr += L"/" + file_in_main.GetFilename();

		WriteProbablyCorruptedFiles(main_path_wstr, bckup_path_wstr);

		main_files.erase(it);
		backup_files.erase(it);
		traversed++;
	}
	stop_ppt = true;
	out.close();
	ppt2.join();

	traversed = 0;
	stop_ppt = false;

	std::cout << "searching for deleted from main files" << std::endl;
	std::set<std::pair<std::wstring, std::uintmax_t>> supposedly_deleted_from_main(mathset<std::pair<std::wstring, std::uintmax_t>>::difference(backup_files, main_files));
	out.open("supposedly deleted from main.txt", std::ios::binary);
	out.write((char*)&UTF16LE::signature, 2);

	std::thread ppt3(print_progress, supposedly_deleted_from_main.size(), std::ref(traversed), std::ref(stop_ppt)); //print progress thread 3
	for (auto& it : supposedly_deleted_from_main) {
		File& file_in_backup{ backup.at(backup_file_hash_map[it]) };
		std::wstring bckup_path_wstr(file_in_backup.GetStartPoint());
		if (file_in_backup.GetRelativePath() != L"") bckup_path_wstr += L"/" + file_in_backup.GetRelativePath();
		bckup_path_wstr += L"/" + file_in_backup.GetFilename();

		out.write((char*)bckup_path_wstr.data(), bckup_path_wstr.size() * 2);
		out.write((char*)&UTF16LE::NewLine, 2);
		traversed++;
	}
	stop_ppt = true;
	out.close();
	ppt3.join();
	std::cout << "Done!" << std::endl;
	return 0;
}