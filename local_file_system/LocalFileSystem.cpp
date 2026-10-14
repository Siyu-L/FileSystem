#include <iostream>
#include <string>
#include <vector>
#include <assert.h>
#include "cstring"

#include "LocalFileSystem.h"
#include "ufs.h"

using namespace std;


LocalFileSystem::LocalFileSystem(Disk *disk) {
  this->disk = disk;
}

void LocalFileSystem::readSuperBlock(super_t *super) {
  char* superBlock = (char*)malloc(UFS_BLOCK_SIZE);
  disk->readBlock(0, superBlock);
  memcpy(super, superBlock, sizeof(super_t));
  free(superBlock);
}

void LocalFileSystem::readInodeBitmap(super_t *super, unsigned char *inodeBitmap) {
  char* bitmapBlock = (char*)malloc(super->inode_bitmap_len*UFS_BLOCK_SIZE);

  for(int i = 0; i < super->inode_bitmap_len; i++) {
    int blockNum = super->inode_bitmap_addr + i;
    disk->readBlock(blockNum, bitmapBlock + (i*UFS_BLOCK_SIZE));
  }

  int bitmapBytes = (super->num_inodes / 8);
  if ((super->num_inodes % 8) != 0) {
    bitmapBytes++;
  }

  memcpy(inodeBitmap, bitmapBlock, bitmapBytes);
  free(bitmapBlock);
}

void LocalFileSystem::writeInodeBitmap(super_t *super, unsigned char *inodeBitmap) {
  char* bitmapBlock = (char*)malloc(super->inode_bitmap_len*UFS_BLOCK_SIZE);
  memset(bitmapBlock, 0, super->inode_bitmap_len*UFS_BLOCK_SIZE);
  
  int bitmapBytes = (super->num_inodes / 8);
  if ((super->num_inodes % 8) != 0) {
    bitmapBytes++;
  }
  memcpy(bitmapBlock, inodeBitmap, bitmapBytes);

  for(int i = 0; i < super->inode_bitmap_len; i++) {
    int blockNum = super->inode_bitmap_addr + i;
    disk->writeBlock(blockNum, bitmapBlock + (i*UFS_BLOCK_SIZE));
  }
  free(bitmapBlock);
}

void LocalFileSystem::readDataBitmap(super_t *super, unsigned char *dataBitmap) {
  char* bitmapBlock = (char*)malloc(super->data_bitmap_len*UFS_BLOCK_SIZE);
  
  for(int i = 0; i < super->data_bitmap_len; i++) {
    int blockNum = super->data_bitmap_addr + i;
    disk->readBlock(blockNum, bitmapBlock + (i*UFS_BLOCK_SIZE));
  }

  int bitmapBytes = (super->num_data / 8);
  if ((super->num_data % 8) != 0) {
    bitmapBytes++;
  }
  memcpy(dataBitmap, bitmapBlock, bitmapBytes);
  free(bitmapBlock);  
}

void LocalFileSystem::writeDataBitmap(super_t *super, unsigned char *dataBitmap) {
  char* bitmapBlock = (char*)malloc(super->data_bitmap_len*UFS_BLOCK_SIZE);
  memset(bitmapBlock, 0, super->data_bitmap_len*UFS_BLOCK_SIZE);

  int bitmapBytes = (super->num_data / 8);
  if ((super->num_data % 8) != 0) {
    bitmapBytes++;
  }
  memcpy(bitmapBlock, dataBitmap, bitmapBytes);

  for(int i = 0; i < super->data_bitmap_len; i++) {
    int blockNum = super->data_bitmap_addr + i;
    disk->writeBlock(blockNum, bitmapBlock + (i*UFS_BLOCK_SIZE));
  }
  free(bitmapBlock);
}

void LocalFileSystem::readInodeRegion(super_t *super, inode_t *inodes) {
  char* inodeRegion = (char*)malloc(super->inode_region_len*UFS_BLOCK_SIZE);

  for(int i = 0; i < super->inode_region_len; i++) {
    int blockNum = super->inode_region_addr + i;
    disk->readBlock(blockNum, ((unsigned char*)inodeRegion) + (i*UFS_BLOCK_SIZE));    
  }
  
  int inodeBytes = super->num_inodes * sizeof(inode_t);
  memcpy(inodes, inodeRegion, inodeBytes);
  free(inodeRegion);
}

void LocalFileSystem::writeInodeRegion(super_t *super, inode_t *inodes) {
  char* inodeRegion = (char*)malloc(super->inode_region_len*UFS_BLOCK_SIZE);
  memset(inodeRegion, 0, super->inode_region_len*UFS_BLOCK_SIZE);

  int inodeBytes = super->num_inodes * sizeof(inode_t);
  memcpy(inodeRegion, inodes, inodeBytes);
  
  for(int i = 0; i < super->inode_region_len; i++) {
    int blockNum = super->inode_region_addr + i;
    disk->writeBlock(blockNum, ((unsigned char*)inodeRegion) + (i*UFS_BLOCK_SIZE));    
  }
  free(inodeRegion);
}

int LocalFileSystem::lookup(int parentInodeNumber, string name) {
  super_t super;
  readSuperBlock(&super);
  if (parentInodeNumber< 0 || parentInodeNumber >= super.num_inodes) {
    return -EINVALIDINODE;
  }  

  inode_t parentInode;
  int ret = stat(parentInodeNumber, &parentInode);
  if (ret != 0) {
    return -EINVALIDINODE;
  }

  if (parentInode.type != UFS_DIRECTORY) {
    return -EINVALIDINODE;
  }

  const char* entName = name.c_str();
  int numEntries = parentInode.size / sizeof(dir_ent_t);
  int entriesPerBlock = UFS_BLOCK_SIZE / sizeof(dir_ent_t);

  dir_ent_t* currBlock = (dir_ent_t*)malloc(UFS_BLOCK_SIZE);

  // find entry in directory blocks
  for (int i = 0; i < numEntries; i++) {
    int entryNum = i % entriesPerBlock;
    // if entry is on new block
    if (entryNum == 0) {
      int blockNum = i / entriesPerBlock;
      disk->readBlock(parentInode.direct[blockNum], currBlock);
    }

    dir_ent_t entry = currBlock[entryNum];
    if (strcmp(entry.name, entName) == 0) {
      free(currBlock);
      return entry.inum;
    }
  }

  free(currBlock);
  return -ENOTFOUND;
}


int LocalFileSystem::stat(int inodeNumber, inode_t *inode) {
  super_t super;
  readSuperBlock(&super);
  if (inodeNumber < 0 || inodeNumber >= super.num_inodes) {
    return -EINVALIDINODE;
  }

  int inode_bitmap_bytes = (super.num_inodes / 8);
  if ((super.num_inodes % 8) != 0) {
    inode_bitmap_bytes++;
  }

  unsigned char* inode_bitmap = (unsigned char*)malloc(inode_bitmap_bytes);
  readInodeBitmap(&super, inode_bitmap);
  

  int inode_byte_num = inodeNumber / 8;
  int inode_bit_offset = inodeNumber % 8;
  if ((inode_bitmap[inode_byte_num] & (1 << inode_bit_offset)) == 0) {
    free(inode_bitmap);
    return -ENOTALLOCATED;
  }
  free(inode_bitmap);


  inode_t* inodes = (inode_t*)malloc(super.inode_region_len * UFS_BLOCK_SIZE);
  readInodeRegion(&super, inodes);

  inode->type = inodes[inodeNumber].type;
  inode->size = inodes[inodeNumber].size;
  for (int i = 0; i < DIRECT_PTRS; i++) {
    inode->direct[i] = inodes[inodeNumber].direct[i];
  }
  free(inodes);
  return 0;
}

int LocalFileSystem::read(int inodeNumber, void *buffer, int size) {
  if (size == 0) return 0;
  if (size < 0 || size > MAX_FILE_SIZE) {
    return -EINVALIDSIZE;
  }

  super_t super;
  readSuperBlock(&super);
  inode_t inode;
  int ret = stat(inodeNumber, &inode);
  if (ret != 0) {
    return ret;
  }

  // if the caller specifies size less than the size of the object, 
  // return only these bytes 
  // If the caller specifies a size of larger than the size of the object, 
  // return the bytes in the object.
  int bytesRead = (size > inode.size) ? inode.size : size;
  int blockNum = 0;
  int temp = bytesRead;
  while(temp >= UFS_BLOCK_SIZE) {
    disk->readBlock(inode.direct[blockNum], ((unsigned char*)buffer) + (blockNum*UFS_BLOCK_SIZE));
    blockNum++;
    temp -= UFS_BLOCK_SIZE;
  }

  // read in last block
  if (temp > 0) {
    char* lastBlock = (char*)malloc(UFS_BLOCK_SIZE);
    disk->readBlock(inode.direct[blockNum], lastBlock);

    // copy remaining data in last block into buffer
    memcpy(((unsigned char*)buffer) + (blockNum*UFS_BLOCK_SIZE), lastBlock, temp);
    free(lastBlock);
  }

  return bytesRead;

}

// find open spot in bitmap
// blockType: 0 = inode, 1 = data
// returns index of open spot if success, returns -ENOTENOUGHSPACE on fail
int LocalFileSystem::find_open_spot(super_t* super, int blockType, unsigned char* bitmap, int bitmapBytes) {
  int super_num = (blockType == 0) ? super->num_inodes : super->num_data;

  int curr_num = -1;
  for (int i = 0; i < bitmapBytes; i++) {
    unsigned int currByte = (unsigned int) bitmap[i];
    for (int j = 0; j < 8; j++) {
      int currBit = (currByte >> j) & 1;
      if (!currBit) {
        curr_num = i*8 + j;
        break;
      }
    }
    if (curr_num != -1) break;
  }

  if (curr_num < 0 || curr_num >= super_num) {
    return -ENOTENOUGHSPACE;
  }
  return curr_num;
}

// finds an open spot and sets it in bitmap
// returns block number on success, negative value on failure
// absolute disk block number = super.data_region_addr + bitmap index
int LocalFileSystem::allocate_data_block(super_t *super) {
  int data_bitmap_bytes = (super->num_data + 7) / 8;
  unsigned char* data_bitmap = (unsigned char*)malloc(data_bitmap_bytes);
  readDataBitmap(super, data_bitmap);
  
  int data_num = find_open_spot(super, 1, data_bitmap, data_bitmap_bytes);
  if (data_num < 0) {
    free(data_bitmap);
    return data_num;
  }

  int data_byte_num = data_num / 8;
  int data_bit_offset = data_num % 8;
  data_bitmap[data_byte_num] |= (1 << data_bit_offset);
  writeDataBitmap(super, data_bitmap);

  free(data_bitmap);
  return super->data_region_addr + data_num;
}

void LocalFileSystem::deallocate_data_block(super_t *super, int blockNumber) {
  if (blockNumber < super->data_region_addr || blockNumber >= super->data_region_addr + super->num_data) {
    return;
  }
  int data_bitmap_bytes = (super->num_data + 7) / 8;
  unsigned char* data_bitmap = (unsigned char*)malloc(data_bitmap_bytes);
  readDataBitmap(super, data_bitmap);
  int rel_block = blockNumber - super->data_region_addr;
  data_bitmap[rel_block / 8] &= ~(1 << (rel_block % 8));
  writeDataBitmap(super, data_bitmap);
  free(data_bitmap);
}

// writes inode to inode region
// assumes inode and inode number is valid, no failure modes
void LocalFileSystem::writeInodeToRegion(super_t* super, const inode_t &inode, int inode_num) {
  int inodeBytes = super->num_inodes * sizeof(inode_t);
  inode_t* inodes = (inode_t*)malloc(inodeBytes);
  readInodeRegion(super, inodes);
  inodes[inode_num] = inode;
  writeInodeRegion(super, inodes);
  free(inodes);
}

int LocalFileSystem::create(int parentInodeNumber, int type, string name) {
  if (name.size() >= DIR_ENT_NAME_SIZE) {
    return -EINVALIDNAME;
  }

  if (type != UFS_DIRECTORY && type != UFS_REGULAR_FILE) {
    return -EINVALIDTYPE;
  }

  super_t super;
  readSuperBlock(&super);
  if (parentInodeNumber< 0 || parentInodeNumber >= super.num_inodes) {
    return -EINVALIDINODE;
  }  

  inode_t parentInode;
  int ret = stat(parentInodeNumber, &parentInode);
  if (ret != 0) {
    return -EINVALIDINODE;
  }

  if (parentInode.type != UFS_DIRECTORY) {
    return -EINVALIDINODE;
  }

  if (parentInode.size + sizeof(dir_ent_t) > MAX_FILE_SIZE) {
    return -ENOTENOUGHSPACE;
  }
  
  // check if name already exists
  int lk_ret = lookup(parentInodeNumber, name);
  if (lk_ret != -ENOTFOUND) {
    if (lk_ret == -EINVALIDINODE) {
      return lk_ret;
    }
    inode_t inode;
    ret = stat(lk_ret, &inode);
    if (ret != 0) {
      return -EINVALIDINODE;
    }    

    if (inode.type == type) {
      return lk_ret;
    }
    else {
      return -EINVALIDTYPE;
    }
  }

  // find open inode spot
  int inode_bitmap_bytes = (super.num_inodes / 8);
  if ((super.num_inodes % 8) != 0) {
    inode_bitmap_bytes++;
  }

  unsigned char* inode_bitmap = (unsigned char*)malloc(inode_bitmap_bytes);
  readInodeBitmap(&super, inode_bitmap);
  
  int inode_num;
  inode_num = find_open_spot(&super, 0, inode_bitmap, inode_bitmap_bytes);
  // error
  if (inode_num < 0) {
    free(inode_bitmap);
    return inode_num;
  }
  
  // set inode bitmap
  int inode_byte_num = inode_num / 8;
  int inode_bit_offset = inode_num % 8;
  inode_bitmap[inode_byte_num] |= (1 << inode_bit_offset);

  // if directory, allocate a disk block
  int data_num;
  if (type == UFS_DIRECTORY) {
    data_num = allocate_data_block(&super);
    // no space error
    if (data_num < 0) {
      // don't commit inode bitmap changes
      free(inode_bitmap);
      return data_num;
    }

  }

  inode_t inode;

  inode.type = type;
  // if file, all pointers initialized to 0
  for (int i = 0; i < DIRECT_PTRS; i++) {
    inode.direct[i] = 0;
  }
  inode.size = 0;
  int dir_init_size;

  // if directory, initialize pointer to data block, has "." and ".." entries
  if (type == UFS_DIRECTORY) {
    // direct pointers point to absolute disk block number
    inode.direct[0] = data_num;
    dir_init_size = sizeof(dir_ent_t) * 2;
    inode.size = dir_init_size;
  }


  // set initial entries in directory
  if (type == UFS_DIRECTORY) {
    dir_ent_t* entries = (dir_ent_t*)malloc(dir_init_size);
    dir_ent_t self = {".", inode_num};
    dir_ent_t parent = {"..", parentInodeNumber};
    entries[0] = self;
    entries[1] = parent;

    char* dataBlock = (char*)malloc(UFS_BLOCK_SIZE);
    memset(dataBlock, 0, UFS_BLOCK_SIZE);
    memcpy(dataBlock, entries, dir_init_size);
    disk->writeBlock(data_num, dataBlock);

    free(entries);
    free(dataBlock);
  }

  // update parent dir size
  int oldSize = parentInode.size;
  parentInode.size += sizeof(dir_ent_t);

  // add entry to parent directory data block
  int entryNum = (parentInode.size / sizeof(dir_ent_t)) - 1;

  dir_ent_t* dirContent = (dir_ent_t*)malloc(parentInode.size);
  ret = read(parentInodeNumber, dirContent, oldSize);
  if (ret < 0) {
    if (type == UFS_DIRECTORY) {
      deallocate_data_block(&super, data_num);
    }
    free(dirContent);
    free(inode_bitmap);
    return ret;
  }
  dir_ent_t entry;
  strcpy(entry.name, name.c_str());  
  entry.inum = inode_num;
  dirContent[entryNum] = entry;


  ret = write(parentInodeNumber, dirContent, parentInode.size, UFS_DIRECTORY);
  if (ret < 0) {
    if (type == UFS_DIRECTORY) {
      deallocate_data_block(&super, data_num);
    }
    free(dirContent);
    free(inode_bitmap);
    return ret;
  }
  free(dirContent);

  writeInodeToRegion(&super, inode, inode_num);

  writeInodeBitmap(&super, inode_bitmap);
  free(inode_bitmap);

  return inode_num;
}

int LocalFileSystem::write(int inodeNumber, const void *buffer, int size) {
  return write(inodeNumber, buffer, size, UFS_REGULAR_FILE);
}

int LocalFileSystem::write(int inodeNumber, const void *buffer, int size, int expectedType) {
  if (size < 0) {
    return -EINVALIDSIZE;
  }

  super_t super;
  readSuperBlock(&super);
  if (inodeNumber < 0 || inodeNumber >= super.num_inodes) {
    return -EINVALIDINODE;
  }

  inode_t inode;
  int ret = stat(inodeNumber, &inode);
  if (ret != 0) {
    return -EINVALIDINODE;
  }

  if (inode.type != expectedType) {
    return -EINVALIDTYPE;
  }

  int newSize = (size > MAX_FILE_SIZE) ? MAX_FILE_SIZE : size;
  
  int oldBlocks = inode.size / UFS_BLOCK_SIZE;
  if ((inode.size % UFS_BLOCK_SIZE) != 0) {
    oldBlocks += 1;
  }
  int newBlocks = newSize / UFS_BLOCK_SIZE;
  if ((newSize % UFS_BLOCK_SIZE) != 0) {
    newBlocks += 1;
  }

  int diff = newBlocks - oldBlocks;
  
  int data_bitmap_bytes = (super.num_data + 7) / 8;
  inode.size = newSize;

  unsigned char* data_bitmap = (unsigned char*)malloc(data_bitmap_bytes);
  readDataBitmap(&super, data_bitmap);
  
  // need to allocate more blocks
  if (diff > 0) {
    for (int i = oldBlocks; i < newBlocks; i++) {
      int data_index = find_open_spot(&super, 1, data_bitmap, data_bitmap_bytes);
      if (data_index < 0) {
        newBlocks = i;
        newSize = (newBlocks == 0) ? 0 : std::min(newSize, newBlocks * UFS_BLOCK_SIZE);
        inode.size = newSize;
        break;
      }
      int data_byte_num = data_index / 8;
      int data_bit_offset = data_index % 8;
      data_bitmap[data_byte_num] |= (1 << data_bit_offset);
      inode.direct[i] = data_index + super.data_region_addr;
    }
    writeDataBitmap(&super, data_bitmap);
  }
  // need to deallocate blocks
  if (diff < 0) {
    for (int i = oldBlocks - 1; i >= newBlocks; i--) {
      int data_num = inode.direct[i] - super.data_region_addr;
      int data_byte_num = data_num / 8;
      int data_bit_offset = data_num % 8;
      data_bitmap[data_byte_num] &= ~(1 << data_bit_offset);
      
      inode.direct[i] = 0;
    }    
    writeDataBitmap(&super, data_bitmap);
  }
  
  char* block = (char*)malloc(UFS_BLOCK_SIZE);
  // copy all filled blocks
  for (int i = 0; i < newBlocks - 1; i++) {
    memcpy(block, (const char*)buffer + (i * UFS_BLOCK_SIZE), UFS_BLOCK_SIZE);
    disk->writeBlock(inode.direct[i], block);
  }
  // remaining partial block
  if (newSize % UFS_BLOCK_SIZE != 0) {
    memset(block, 0, UFS_BLOCK_SIZE);
    memcpy(block, (const char*)buffer + ((newBlocks - 1) * UFS_BLOCK_SIZE), newSize % UFS_BLOCK_SIZE);
    disk->writeBlock(inode.direct[newBlocks - 1], block);
  }
  // remaining full block
  else if (newSize != 0) {
    memcpy(block, (const char*)buffer + ((newBlocks - 1) * UFS_BLOCK_SIZE), UFS_BLOCK_SIZE);
    disk->writeBlock(inode.direct[newBlocks - 1], block);
  }
  
  writeInodeToRegion(&super, inode, inodeNumber);

  free(data_bitmap);
  free(block);
  return newSize;
}

int LocalFileSystem::unlink(int parentInodeNumber, string name) {
  if (name.size() >= DIR_ENT_NAME_SIZE) {
    return -EINVALIDNAME;
  }
  if (name == "." || name == "..") {
    return -EUNLINKNOTALLOWED;
  }

  super_t super;
  readSuperBlock(&super);
  if (parentInodeNumber< 0 || parentInodeNumber >= super.num_inodes) {
    return -EINVALIDINODE;
  }  

  
  int inum = lookup(parentInodeNumber, name);
  if (inum == -EINVALIDINODE) {
    return inum;
  }
  // name not found, does not count as failure
  else if (inum == -ENOTFOUND) {
    return 0;
  }


  inode_t parentInode;
  int ret = stat(parentInodeNumber, &parentInode);
  if (ret < 0) {
    return ret;
  }

  inode_t inode;
  ret = stat(inum, &inode);
  if (ret < 0) {
    return ret;
  }

  // if directory, check if its not empty
  if (inode.type == UFS_DIRECTORY) {
    if(inode.size != 2*sizeof(dir_ent_t)) {
      return -EDIRNOTEMPTY; 
    }
    // clear data blocks
    ret = write(inum, nullptr, 0, UFS_DIRECTORY);
    if (ret < 0) {
      return ret;
    }
  }
  if (inode.type == UFS_REGULAR_FILE) {
    // clear data blocks
    ret = write(inum, nullptr, 0);
    if (ret < 0) {
      return ret;
    }
  }

  // remove entry from parent directory
  int removeNum = -1;
  const char* entName = name.c_str();
  int numEntries = parentInode.size / sizeof(dir_ent_t);

  dir_ent_t* dirContent = (dir_ent_t*)malloc(parentInode.size);
  ret = read(parentInodeNumber, dirContent, parentInode.size);
  if (ret < 0) {
    free(dirContent);
    return ret;
  }

  // find entry
  for (int i = 0; i < numEntries; i++) {      
    dir_ent_t entry = dirContent[i];
    if (strcmp(entry.name, entName) == 0) {
      removeNum = i;
      break;
    }
  }
  if (removeNum == -1) {
    free(dirContent);
    return 0;
  }

  // shift all entry after it left
  for (int i = removeNum; i < numEntries - 1; i++) {
    dirContent[i] = dirContent[i+1];
  }
  parentInode.size -= sizeof(dir_ent_t);

  ret = write(parentInodeNumber, dirContent, parentInode.size, UFS_DIRECTORY);
  if (ret < 0) {
    free(dirContent);
    return ret;
  }
  free(dirContent);



        
  // set inode bitmap
  int inode_bitmap_bytes = (super.num_inodes / 8);
  if ((super.num_inodes % 8) != 0) {
    inode_bitmap_bytes++;
  }
  unsigned char* inode_bitmap = (unsigned char*)malloc(inode_bitmap_bytes);
  readInodeBitmap(&super, inode_bitmap);
    
  int inode_byte_num = inum / 8;
  int inode_bit_offset = inum % 8;
  inode_bitmap[inode_byte_num] &= ~(1 << inode_bit_offset);
  writeInodeBitmap(&super, inode_bitmap);
  
  free(inode_bitmap);
  

  return 0;
}

