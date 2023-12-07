// ============================================================================
// fs.c - user FileSytem API
// ============================================================================

#include "bfs.h"
#include "fs.h"

// ============================================================================
// Close the file currently open on file descriptor 'fd'.
// ============================================================================
i32 fsClose(i32 fd) { 
  i32 inum = bfsFdToInum(fd);
  bfsDerefOFT(inum);
  return 0; 
}



// ============================================================================
// Create the file called 'fname'.  Overwrite, if it already exsists.
// On success, return its file descriptor.  On failure, EFNF
// ============================================================================
i32 fsCreate(str fname) {
  i32 inum = bfsCreateFile(fname);
  if (inum == EFNF) return EFNF;
  return bfsInumToFd(inum);
}



// ============================================================================
// Format the BFS disk by initializing the SuperBlock, Inodes, Directory and 
// Freelist.  On succes, return 0.  On failure, abort
// ============================================================================
i32 fsFormat() {
  FILE* fp = fopen(BFSDISK, "w+b");
  if (fp == NULL) FATAL(EDISKCREATE);

  i32 ret = bfsInitSuper(fp);               // initialize Super block
  if (ret != 0) { fclose(fp); FATAL(ret); }

  ret = bfsInitInodes(fp);                  // initialize Inodes block
  if (ret != 0) { fclose(fp); FATAL(ret); }

  ret = bfsInitDir(fp);                     // initialize Dir block
  if (ret != 0) { fclose(fp); FATAL(ret); }

  ret = bfsInitFreeList();                  // initialize Freelist
  if (ret != 0) { fclose(fp); FATAL(ret); }

  fclose(fp);
  return 0;
}


// ============================================================================
// Mount the BFS disk.  It must already exist
// ============================================================================
i32 fsMount() {
  FILE* fp = fopen(BFSDISK, "rb");
  if (fp == NULL) FATAL(ENODISK);           // BFSDISK not found
  fclose(fp);
  return 0;
}



// ============================================================================
// Open the existing file called 'fname'.  On success, return its file 
// descriptor.  On failure, return EFNF
// ============================================================================
i32 fsOpen(str fname) {
  i32 inum = bfsLookupFile(fname);        // lookup 'fname' in Directory
  if (inum == EFNF) return EFNF;
  return bfsInumToFd(inum);
}



// ============================================================================
// Read 'numb' bytes of data from the cursor in the file currently fsOpen'd on
// File Descriptor 'fd' into 'buf'.  On success, return actual number of bytes
// read (may be less than 'numb' if we hit EOF).  On failure, abort
// ============================================================================
i32 fsRead(i32 fd, i32 numb, void* buf) {
  //check how much to read to neg and file size
  if(numb <= 0){ FATAL(ENEGNUMB); }
  i32 fSize = fsSize(fd);
  if(numb > fSize){ FATAL(EBIGNUMB); }
  
  //setup cursor for reading
  i32 cursorPos = fsTell(fd); //get the files cursor position
  i32 startRead = cursorPos; //where to start reading
  i32 endRead = startRead + numb; //where to stop reading
  //check if cursor is valid
  if(cursorPos < 0 || cursorPos >= (BYTESPERBLOCK * BLOCKSPERDISK)){ FATAL(EBADCURS); }

  //check if numb from cursor goes past EOF
  i32 numRead = numb;
  if(endRead > fSize){
    numRead = fSize - startRead;
    endRead = startRead + numRead;
  }
  
  //setup read buffer
  i32 startFbn = startRead / BYTESPERBLOCK;
  i32 endFBN = endRead / BYTESPERBLOCK;
  i8 readBuffer[(endFBN - startFbn + 1) * BYTESPERBLOCK];
  i8 tempBuffer[BYTESPERBLOCK];
  i32 offset = 0;
  i32 inum = bfsFdToInum(fd); //get inum to the file

  //read from disk into buffer
  for(i32 i = startFbn; i <= endFBN; i++){
    i32 read = bfsRead(inum, i, tempBuffer);
    if(read > 0 || read < 0){ //bad read error handling
      FATAL(EBADREAD);
    }
    memcpy((readBuffer + offset), tempBuffer, BYTESPERBLOCK);
    //printf("cursor: %d\n", fsTell(fd));
    offset += BYTESPERBLOCK;
  }

  //move into og buffer
  memcpy(buf, (readBuffer + (cursorPos % BYTESPERBLOCK)), numRead);
  fsSeek(fd, numRead, SEEK_CUR);

  return numRead;
}


// ============================================================================
// Move the cursor for the file currently open on File Descriptor 'fd' to the
// byte-offset 'offset'.  'whence' can be any of:
//
//  SEEK_SET : set cursor to 'offset'
//  SEEK_CUR : add 'offset' to the current cursor
//  SEEK_END : add 'offset' to the size of the file
//
// On success, return 0.  On failure, abort
// ============================================================================
i32 fsSeek(i32 fd, i32 offset, i32 whence) {

  if (offset < 0) FATAL(EBADCURS);
 
  i32 inum = bfsFdToInum(fd);
  i32 ofte = bfsFindOFTE(inum);
  
  switch(whence) {
    case SEEK_SET:
      g_oft[ofte].curs = offset;
      break;
    case SEEK_CUR:
      g_oft[ofte].curs += offset;
      break;
    case SEEK_END: {
      i32 end = fsSize(fd);
      g_oft[ofte].curs = end + offset;
      break;
    }
    default:
      FATAL(EBADWHENCE);
  }
  return 0;
}



// ============================================================================
// Return the cursor position for the file open on File Descriptor 'fd'
// ============================================================================
i32 fsTell(i32 fd) {
  return bfsTell(fd);
}



// ============================================================================
// Retrieve the current file size in bytes.  This depends on the highest offset
// written to the file, or the highest offset set with the fsSeek function.  On
// success, return the file size.  On failure, abort
// ============================================================================
i32 fsSize(i32 fd) {
  i32 inum = bfsFdToInum(fd);
  return bfsGetSize(inum);
}



// ============================================================================
// Write 'numb' bytes of data from 'buf' into the file currently fsOpen'd on
// filedescriptor 'fd'.  The write starts at the current file offset for the
// destination file.  On success, return 0.  On failure, abort
// ============================================================================
i32 fsWrite(i32 fd, i32 numb, void* buf) {
  //setup
  i32 cursor = fsTell(fd);
  i32 startFBN = cursor / BYTESPERBLOCK;
  i32 endFBN = (cursor + numb + 1) / BYTESPERBLOCK;
  i32 inum = bfsFdToInum(fd);
  i32 blockCount = endFBN - startFBN + 1;

  //some error handling
  if(numb <= 0){ FATAL(ENEGNUMB); }
  if(numb >= (BYTESPERBLOCK * BLOCKSPERDISK)){ FATAL(EBIGNUMB); }
  if(cursor < 0 || cursor >= (BYTESPERBLOCK * BLOCKSPERDISK)){ FATAL(EBADCURS); }
  if(blockCount > 5) { FATAL(EBADFBN); }

  //check if file size is ok, if not, make adjustments
  if(endFBN > ((fsSize(fd) - 1) / BYTESPERBLOCK)){
    bfsExtend(inum, endFBN);
    bfsSetSize(inum, cursor + numb);
  }
  
  //setup read/write buffers
  i8 bioBuff[blockCount * BYTESPERBLOCK];
  i8 tempBuff[BYTESPERBLOCK];

  //copy first and last block, edge holders
  i32 bad = bfsRead(inum, startFBN, tempBuff);
  if(bad < 0 || bad > 0) { FATAL(EBADREAD); } //check read was good
  memcpy(bioBuff, tempBuff, BYTESPERBLOCK);

  if(blockCount > 1){
    bad = bfsRead(inum, endFBN, tempBuff);
    if(bad < 0 ||  bad > 0) { FATAL(EBADREAD); } //check read was good
    memcpy(bioBuff + (blockCount - 1) * BYTESPERBLOCK, tempBuff, BYTESPERBLOCK);
    memcpy(bioBuff + cursor % BYTESPERBLOCK, buf, numb);

    //get FBN to DBN
    i32 dbn = ENODBN;
    i32 offset = 0;
    
    for(i32 i = startFBN; i <= endFBN; i++){
      memcpy(tempBuff, bioBuff + offset, BYTESPERBLOCK);
      dbn = bfsFbnToDbn(inum, i);
      //printf("dbn: %d\n", dbn);
      if(dbn == ENODBN) { FATAL(EBADDBN); } //bad dbn
      bad = bioWrite(dbn, tempBuff);
      if(bad < 0 || bad > 0) { FATAL(EBADWRITE); } //check for bad write
      offset += BYTESPERBLOCK;
    }
  }

  fsSeek(fd, numb, SEEK_CUR); //move cursor to new pos
  return 0;
}