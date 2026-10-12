#include <iostream>
#include <string>

#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <unistd.h>

#include "LocalFileSystem.h"
#include "Disk.h"
#include "ufs.h"

using namespace std;
#define BUFFERSIZE 100

int main(int argc, char *argv[]) {
  if (argc != 4) {
    cerr << argv[0] << ": diskImageFile src_file dst_inode" << endl;
    cerr << "For example:" << endl;
    cerr << "    $ " << argv[0] << " tests/disk_images/a.img dthread.cpp 3" << endl;
    return 1;
  }

  // Parse command line arguments

  Disk *disk = new Disk(argv[1], UFS_BLOCK_SIZE);
  LocalFileSystem *fileSystem = new LocalFileSystem(disk);
  string srcFile = string(argv[2]);
  int dstInode = stoi(argv[3]);
  
  int fd = open(srcFile.c_str(), O_RDONLY);
  if (fd < 0) {
    cerr << "Cannot open file" << endl;
    delete disk;
    delete fileSystem;
    exit(1);
  }
  
  struct stat st;
  if (fstat(fd, &st) == -1) {
    cerr << "fstat failed" << endl;
    delete disk;
    delete fileSystem;    
    exit(1);
  }

  int fileSize = st.st_size;

  char* buffer = (char*)malloc(fileSize);
  int bytes_read = read(fd, buffer, fileSize);
  if (bytes_read < 0) {
    cerr << "Cannot read file" << endl;
    free(buffer);
    delete disk;
    delete fileSystem;    
    exit(1);
  }

  int ret = fileSystem->write(dstInode, buffer, bytes_read);
  if (ret < 0) {
    cerr << "Could not write to dst_file" << endl;
    close(fd);
    free(buffer);
    delete disk;
    delete fileSystem;

    exit(1);
  }

  close(fd);
  free(buffer);
  delete disk;
  delete fileSystem;

  return 0;
}
