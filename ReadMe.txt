Description:
This project is aimed to synchronize files on two start points within a local machine. Start point is a directory from which all files and directories are compared with those in another start point. For example, you have two directories with plenty of files (like "/usr/main" and "/usr/backup", or "C:/usr/main" and "C:/usr/backup"). And some of these cases:
1) "usr/main" contains fresh (current) versions of files, and "usr/backup" contains old versions of the same files;
2) "usr/main" contains renamed and relocated versions of files, and you want the same arrangement in "usr/backup";
3) "usr/main" contains recently created files that "usr/backup" doesn't;
4) part of files in "usr/main" suddenly became corrupted, so you want to compare them with those in "usr/backup" and recover what is necessary.
Not to mention that some files in "usr/main" or "usr/backup" are pure duplicates, merely spreaded over various directories.
If you are faced with some of these challenges, welcome!

How to use (Input):
Create UTF16LE (2-byte unicode little endian) text file. Fill it with the following arguments, each per line, in the fixed order:
main start point (path to the directory with the last state of files)
backup start point (path to the directory with the old state of main start point)
f|t (fast or thorough mode)

Then pass this filename as the input argument, for example:
SynchronizeData config.txt

Output:
1) New (recently added) files in main that are not in backup, are copied into backup at the same relative (to start point) locations as main.
2) Unique (by contents) files in backup are renamed and relocated in accordance to the same (by contents) files in main (if they are also unique)
3) Non-unique files in backup are copied into new backup's locations in accordance to the same (by contents) files in main
All ambiguities and fails are written into generated files:
"Files with long paths.txt". Contains files which path is more than 256 bytes - they cannot be copied or moved.
"Not copied files.txt". Contains list of files that haven't been copied.
"Possibly corrupted files.txt". Contains pairs of files that have the same names and relative locations, but different contents. There can be two reasons. Either file in main is newer than in backup, or one of the files is corrupted. They should be compared and handled as it seem relevant to you.
"Possibly deleted from main files.txt". Contains list of files existing in backup but absent in main. It means that those files were either moved in another location, or removed at all. Probably they should be deleted - it's up to you.

How it works:
1) The program traverses all files from main start point and backup start point. Then it arranges filenames and relative locations in three sets:
	a) A set of files that are found in main but not found in backup (on the same relative locations and under the same names)
	b) A set of files that are found in both main and backup (on the same relative locations and under the same names)
	c) A set of files that are found in backup but not found in main (on the same relative locations and under the same names)
The difference between fast and thorough mode is that in the thorough mode files in the set (b) are compared by contents to check if some of them were corrupted or changed. In fast mode the checking is omitted.

2) The sets (a) and (c) are transformed into multimaps of type {<size, extension> -> filename} to identify files with the same size and extension. Unique files with the same size and extension are compared, and if contents is equal, the file in backup is relocated and renamed in accordance to the file in main. Otherwise it's written into "Not copied files.txt".

3) Multimaps are transformed into new multimaps of type {<contents hash> -> filename} to separate files that may have the same size and extension, but different contents. Now the existence of one-off files with the same hash on main and backup leads to relocation and renaming in backup. The remaining files in main are copied into backup, so that if the same file exists in backup, it's copied into backup from location in backup (not from location in main), to save time. The remaining (non-unique) files in backup are written into the file "Possibly deleted from main files.txt".