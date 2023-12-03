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
  i8 tBlocks = endFBN - startFbn + 1; //inc for indirect
  i8 readBuffer[tBlocks * BYTESPERBLOCK];
  i8 tempBuffer[BYTESPERBLOCK];
  i32 offset = 0;

  i32 inum = bfsFdToInum(fd); //get inum to the file

  //read from disk into buffer
  bool bad = false;
  for(i32 i = startFbn; i <= endFBN; i++){
    if(bfsRead(inum, i, tempBuffer) > 0){ //bad read
      bad = true; 
      break;
    }
    memcpy((readBuffer + offset), tempBuffer, BYTESPERBLOCK);
    //printf("cursor: %d\n", fsTell(fd));
    offset += BYTESPERBLOCK;
  }

  //return bad read
  if(bad){ FATAL(EBADREAD); }

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



  FATAL(ENYI);                                  // Not Yet Implemented!
  return 0;
}
