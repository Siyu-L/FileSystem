#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cstring>

#include "LocalFileSystem.h"
#include "Disk.h"
#include "ufs.h"

using namespace std;

int main(int argc, char *argv[]) {
  if (argc != 3) {
    cerr << argv[0] << ": diskImageFile inodeNumber" << endl;
    return 1;
  }

  // Parse command line arguments

  Disk *disk = new Disk(argv[1], UFS_BLOCK_SIZE);
  LocalFileSystem *fileSystem = new LocalFileSystem(disk);

  int inode_num;
  try {
    inode_num = stoi(argv[2]);
  }
  catch (const std::exception& e) {
    cerr << "Error reading file" << endl;
    exit(1);
  }

  // check if inode is directory
  inode_t inode;
  int ret = fileSystem->stat(inode_num, &inode);
  if (ret < 0) {
    cerr << "Error reading file" << endl;
    exit(1);    
  }
  if (inode.type == UFS_DIRECTORY) {
    cerr << "Error reading file" << endl;
    exit(1);
  }

  int blocks = inode.size / UFS_BLOCK_SIZE;
  if ((inode.size % UFS_BLOCK_SIZE) != 0) {
    blocks += 1;
  }
  cout << "File blocks" << endl;
  for (int i = 0; i < blocks; i++) {
    cout << inode.direct[i] << endl;
  }
  cout << endl;
  cout << "File data" << endl;
  int bytes_read;
  vector<char> buffer(inode.size+1);
  bytes_read = fileSystem->read(inode_num, buffer.data(), inode.size);
  if (bytes_read < 0) {
    cerr << "Error reading file" << endl;
    exit(1);    
  }
  buffer[inode.size] = '\0';
  string output = buffer.data();
  cout << output;
  
  delete disk;
  delete fileSystem;

  return 0;

}
