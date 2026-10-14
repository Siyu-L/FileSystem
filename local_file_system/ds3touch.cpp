#include <iostream>
#include <string>
#include <vector>

#include "LocalFileSystem.h"
#include "Disk.h"
#include "ufs.h"

using namespace std;

int main(int argc, char *argv[]) {
  if (argc != 4) {
    cerr << argv[0] << ": diskImageFile parentInode fileName" << endl;
    cerr << "For example:" << endl;
    cerr << "    $ " << argv[0] << " a.img 0 a.txt" << endl;
    return 1;
  }

  // Parse command line arguments
  Disk *disk = new Disk(argv[1], UFS_BLOCK_SIZE);
  LocalFileSystem *fileSystem = new LocalFileSystem(disk);
  int parentInode;
  try {
    parentInode = stoi(argv[2]);
  } catch (...) {
    cerr << "Error creating file" << endl;
    delete disk;
    delete fileSystem;
    exit(1);
  }
  string fileName = string(argv[3]);

  int ret = fileSystem->create(parentInode, UFS_REGULAR_FILE, fileName);
  if (ret < 0) {
    cerr << "Error creating file" << endl;
    delete disk;
    delete fileSystem;
    exit(1);
  }
  delete disk;
  delete fileSystem;

  
  return 0;
}
