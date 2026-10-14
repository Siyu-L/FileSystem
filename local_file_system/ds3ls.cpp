#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cstring>

#include "StringUtils.h"
#include "LocalFileSystem.h"
#include "Disk.h"
#include "ufs.h"

using namespace std;

/*
  Use this function with std::sort for directory entries
*/
bool compareByName(const dir_ent_t& a, const dir_ent_t& b) {
    return std::strcmp(a.name, b.name) < 0;
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    cerr << argv[0] << ": diskImageFile directory" << endl;
    cerr << "For example:" << endl;
    cerr << "    $ " << argv[0] << " tests/disk_images/a.img /a/b" << endl;
    return 1;
  }


  Disk *disk = new Disk(argv[1], UFS_BLOCK_SIZE);
  LocalFileSystem *fileSystem = new LocalFileSystem(disk);
  string directory = string(argv[2]);


  // parse path
  vector<string> dir_path;
  size_t pos = 0;
  string token;
  while ((pos = directory.find("/")) != string::npos) {
    token = directory.substr(0, pos);
    if (!token.empty()) dir_path.push_back(token);
    directory.erase(0, pos + 1);
  }
  if (!directory.empty())  dir_path.push_back(directory);

  // find directory
  int curr_inum = UFS_ROOT_DIRECTORY_INODE_NUMBER;
  int final_dir_inum = UFS_ROOT_DIRECTORY_INODE_NUMBER;
  for (size_t i = 0; i < dir_path.size(); i++) {
    string name = dir_path[i];
    int next_inum = fileSystem->lookup(curr_inum, name);
    if (next_inum < 0) {
      cerr << "Directory not found" << endl;
      exit(1);
    }
    final_dir_inum = curr_inum;
    curr_inum = next_inum;
  }

  inode_t inode;
  int stat_ret = fileSystem->stat(curr_inum, &inode);
  if (stat_ret < 0) {
    cerr << "Directory not found" << endl;
    exit(1);
  }
  vector<dir_ent_t> entries;
  
  // handle directory
  if (inode.type == UFS_DIRECTORY) {
    dir_ent_t* dir_content = (dir_ent_t*)malloc(inode.size);
    fileSystem->read(curr_inum, dir_content, inode.size);
    int numEntries = inode.size / sizeof(dir_ent_t);
    for (int i = 0; i < numEntries; i++) {
      dir_ent_t entry = dir_content[i];
      entries.push_back(entry);
    }
    sort(entries.begin(), entries.end(), compareByName);
    for (auto e:entries) {
      cout << e.inum << "\t" << e.name << endl;
    }
    free(dir_content);
  }

  // handle file
  if (inode.type == UFS_REGULAR_FILE) {
    string filename = dir_path[dir_path.size()-1];
    int file_inum = fileSystem->lookup(final_dir_inum, filename);
    cout << file_inum << "\t" << filename << endl;
  }

  delete disk;
  delete fileSystem;
  
  return 0;
}
