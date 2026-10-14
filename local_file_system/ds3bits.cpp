#include <iostream>
#include <string>
#include <algorithm>
#include <cstring>

#include "LocalFileSystem.h"
#include "Disk.h"
#include "ufs.h"

using namespace std;


int main(int argc, char *argv[]) {
  if (argc != 2) {
    cerr << argv[0] << ": diskImageFile" << endl;
    return 1;
  }

  // Parse command line arguments

  Disk *disk = new Disk(argv[1], UFS_BLOCK_SIZE);
  LocalFileSystem *fileSystem = new LocalFileSystem(disk);
  
  super_t super;
  fileSystem->readSuperBlock(&super);
  cout << "Super" << endl;
  cout << "inode_region_addr " << super.inode_region_addr << endl;
  cout << "inode_region_len " << super.inode_region_len << endl;
  cout << "num_inodes " << super.num_inodes << endl;
  cout << "data_region_addr " << super.data_region_addr << endl;
  cout << "data_region_len " << super.data_region_len << endl;
  cout << "num_data " << super.num_data << endl;
  cout << endl;

  // print inode bitmap
  cout << "Inode bitmap" << endl;
  int inode_bitmap_bytes = (super.num_inodes + 7) / 8;
  unsigned char* inode_bitmap = (unsigned char*)malloc(inode_bitmap_bytes);
  fileSystem->readInodeBitmap(&super, inode_bitmap);
  for (int i = 0; i < inode_bitmap_bytes; i++) {
    cout << (unsigned int) inode_bitmap[i] << " ";
  }
  cout << endl << endl;

  // print data bitmap
  cout << "Data bitmap" << endl;
  int data_bitmap_bytes = (super.num_data + 7) / 8;
  unsigned char* data_bitmap = (unsigned char*)malloc(data_bitmap_bytes);
  fileSystem->readDataBitmap(&super, data_bitmap);
  for (int i = 0; i < data_bitmap_bytes; i++) {
    cout << (unsigned int) data_bitmap[i] << " ";
  }
  cout << endl;

  free(inode_bitmap);
  free(data_bitmap);
  delete disk;
  delete fileSystem;

  return 0;
}
