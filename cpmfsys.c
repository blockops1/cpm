#include "cpmfsys.h"

// function to write contents of a DirStructType struct back to the specified index of the extent
// in block of memory (disk block 0) pointed to by e
void writeDirStruct(DirStructType *d, uint8_t index, uint8_t *e);

// populate the FreeList global data structure. freeList[i] == true means
// that block i of the disk is free. block zero is never free, since it holds
// the directory. freeList[i] == false means the block is in use.
void makeFreeList() {}

// debugging function, print out the contents of the free list in 16 rows of 16, with each
// row prefixed by the 2-digit hex address of the first block in that row. Denote a used
// block with a *, a free block with a .
void printFreeList() {}

// internal function, returns -1 for illegal name or name not found
// otherwise returns extent nunber 0-31
int findExtentWithName(char *name, uint8_t *block0);

// internal function, returns true for legal name (8.3 format), false for illegal
// (name or extension too long, name blank, or  illegal characters in name or extension)
bool checkLegalName(char *name);

// print the file directory to stdout. Each filename should be printed on its own line,
// with the file size, in base 10, following the name and extension, with one space between
// the extension and the size. If a file does not have an extension it is acceptable to print
// the dot anyway, e.g. "myfile. 234" would indicate a file whose name was myfile, with no
// extension and a size of 234 bytes. This function returns no error codes, since it should
// never fail unless something is seriously wrong with the disk
void cpmDir() {
    printf("DIRECTORY LISTING\n");
    uint8_t buffer[1024] = {0};
    uint8_t *buffer_p = NULL;
    buffer_p = (uint8_t * ) & buffer;
    blockRead(buffer_p, 0);
    int line = 0;
    char trimName[9];
    char trimExt[4];
    int length = 0;
    DirStructType entry = {0};
    DirStructType *entry_p = NULL;
    entry_p = &entry;
    for (line = 0; line < BLOCK_SIZE / EXTENT_SIZE; line++) {
        //printf("line: %d\n", line);
        // this section prints the directory entry
        if (buffer[line * EXTENT_SIZE] != 0xe5) {
            fillExtent(entry_p, buffer_p, line);
            strcpy(trimName, entry.name);
            strcpy(trimExt, entry.extension);
            length = trimString(trimName);
            //printf("length: %d", length);
            printf("%.*s.", length, trimName);
            length = trimString(trimExt);
            //printf("length: %d\n", length);
            printf("%.*s ", length, trimExt);
            printf("%d\n", fileSize(entry_p));
            memset(trimName, 0, sizeof(trimName));
            memset(trimExt, 0, sizeof(trimExt));
        }
        //zero out the structure
        memset(&entry, 0, sizeof(entry));
    }
};

// fill an Extent with information from a block from a specific line
int fillExtent(DirStructType *entry_p, uint8_t *block, int extentNum) {
    int line = extentNum * EXTENT_SIZE;
    //go through the block to fill the entry
    entry_p->status = block[line];
    entry_p->BC = block[line + 13];
    entry_p->RC = block[line + 15];
    for (int i = 0; i < 7; i++) {
        entry_p->name[i] = block[line + 1 + i];
    }
    for (int i = 0; i < 3; i++) {
        entry_p->extension[i] = block[line + 9 + i];
    }
    for (int i = 0; i < 32; i++) {
        entry_p->blocks[i] = block[line + 16 + i];
    }
    return 0;
}

// calculate and return the filesize of a directory entry
int fileSize(DirStructType *entry) {
    // count the number of full blocks
    int blockNumber = 0;
    int size = 0;
    while (blockNumber < 16) {
        if (entry->blocks[blockNumber] != 0) {
            size += 1024;
        }
        blockNumber++;
    }
    // calculate bytes in the last block
    size += entry->RC * 128 + entry->BC - 1024;
    // return the sum
    return size;
}

//trims trailing whitespace from string for output. returns size of output string
int trimString(char *word) {
    int length = 0;
    int size = strlen(word);
    char word2[size];
    for (int i = 0; i < size; i++) {
        if (!isspace(word[i])) {
            word2[i] = word[i];
            //printf("%c", word[i]);
            length++;
            //printf("lth: %d\n", length);
        }
    }
    //printf("\n");
    word = word2;
    return length;
}

int lastBlock() {
    return 0;
}

// error codes for next five functions (not all errors apply to all 5 functions)
/* 
    0 -- normal completion
   -1 -- source file not found
   -2 -- invalid  filename
   -3 -- dest filename already exists 
   -4 -- insufficient disk space 
*/

//read directory block,
// modify the extent for file named oldName with newName, and write to the disk
int cpmRename(char *oldName, char *newName) {
    return 0;
}

// delete the file named name, and free its disk blocks in the free list
int cpmDelete(char *name) {
    return 0;
}

// following functions need not be implemented for Lab 2

int cpmCopy(char *oldName, char *newName) {
    return 0;
}

int cpmOpen(char *fileName, char mode) {
    return 0;
}

// non-zero return indicates filePointer did not point to open file
int cpmClose(int filePointer) {
    return 0;
}

// returns number of bytes read, 0 = error
int cpmRead(int pointer, uint8_t *buffer, int size) {
    return 0;
}

// returns number of bytes written, 0 = error
int cpmWrite(int pointer, uint8_t *buffer, int size) {
    return 0;
}
