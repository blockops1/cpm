#include "cpmfsys.h"

//function to allocate memory for a DirStructType (see above), and populate it, given a
//pointer to a buffer of memory holding the contents of disk block 0 (e), and an integer index
// which tells which extent from block zero (extent numbers start with 0) to use to make the
// DirStructType value to return.
DirStructType *mkDirStruct(int index,uint8_t *e){
    int line = index * EXTENT_SIZE;
    DirStructType entry = {0};
    DirStructType *entry_p = NULL;
    entry_p = &entry;
    //go through the block to fill the entry
    entry.status = e[line];
    entry.XL = e[line + 12];
    entry.BC = e[line + 13];
    entry.XH = e[line + 14];
    entry.RC = e[line + 15];
    for (int i = 0; i < 8; i++) {
        entry.name[i] = e[line + 1 + i];
    }
    for (int i = 0; i < 3; i++) {
        entry.extension[i] = e[line + 9 + i];
    }
    for (int i = 0; i < 32; i++) {
        entry.blocks[i] = e[line + 16 + i];
    }
    return entry_p;
}


// function to write contents of a DirStructType struct back to the specified index of the extent
// in block of memory (disk block 0) pointed to by e
void writeDirStruct(DirStructType *d, uint8_t index, uint8_t *e) {
    // check the values before writing to the block
    // check status between 0 and 15
    if (d->status < 0 || d->status > 15) return;
    // check BC
    if (d->BC < 0 || d->BC > 127) return;
    // check RC
    if (d->RC < 0 || d->RC > 7) return;
    // test name
    char nameTest[13];
    for (int i = 0; i < 8; i++) {
        nameTest[i] = d->name[i];
    }
    nameTest[8] = 46;
    for (int i = 0; i < 3; i++) {
        nameTest[i + 9] = d->extension[i];
    }
    printf("Name: %s\n", nameTest);
    if (!checkLegalName(nameTest)) {
        printf("checkLegalNAme failed\n");
        return;
    }
    // test valid blocks
    for (int i = 0; i < 32; i++) {
        if (d->blocks[i] < 0 || d->blocks[i] > 0xFF) {
            printf("illegal block number\n");
            return;
        }
    }
    // passed all checks, can write to block 0;
    int line = index * EXTENT_SIZE;
    e[line] = d->status;
    e[line + 13] = d->BC;
    e[line + 15] = d->RC;
    for (int i = 0; i < 8; i++) {
        e[line + 1 + i] = d->name[i];
    }
    for (int i = 0; i < 3; i++) {
        e[line + 9 + i] = d->extension[i];
    }
    for (int i = 0; i < 32; i++) {
        e[line + 16 + i] = d->blocks[i];
    }
    return;
}

// populate the FreeList global data structure. freeList[i] == true means
// that block i of the disk is free. block zero is never free, since it holds
// the directory. freeList[i] == false means the block is in use.
void makeFreeList() {}

// debugging function, print out the contents of the free list in 16 rows of 16, with each
// row prefixed by the 2-digit hex address of the first block in that row. Denote a used
// block with a *, a free block with a .
void printFreeList() {}

// internal function, returns -1 for illegal name or name not found
// otherwise returns extent number 0-31
int findExtentWithName(char *name, uint8_t *block0);

// internal function, returns true for legal name (8.3 format), false for illegal
// (name or extension too long, name blank, or  illegal characters in name or extension)
bool checkLegalName(char *name) {
    bool valid = true;
    //printf("%s\n", name);
    if (name == NULL) return false;
    if (strlen(name) != 12) return false;
    if (name[8] != *".") return false;
    // check first character 0-9, a-z, A-Z
    valid = legalCharacter(name[0]);
    //check other characters 0-9, a-z, A-Z. If a space, rest of name and ext needs to be space
    int counter = 0;
    while(valid && counter < 7) {
        counter++;
        valid = legalCharacter(name[counter]);
    }
    //printf("line 37 %s\n", valid ? "true" : "false");
    if (counter < 8 && name[counter] == 32) {
        valid = true;
        counter++;
    }
    bool spaceOnly = valid;
    //printf("counter: %d\n", counter);
    //printf("line 40 %s\n", valid ? "true" : "false");
    while(counter < 7) {
        counter++;
        if (name[counter] != 32) spaceOnly = false;
    }
    valid = spaceOnly;
    //printf("counter: %d\n", counter);
    // counter must be 7 by here, now we check the extension
    counter++;
    //printf("line 48 %s\n", valid ? "true" : "false");
    //printf("counter: %d\n", counter);
    while(valid && counter < 11) {
        counter++;
        valid = legalCharacter(name[counter]);
    }
    //printf("line 54 %s\n", valid ? "true" : "false");
    if (counter < 12 && name[counter] == 32) valid = true;
    spaceOnly = valid;
    if (counter == 9 && name[counter] == 32) {
        spaceOnly = true;
        while (spaceOnly && counter < 11) {
            counter++;
            if (name[counter] != 32) spaceOnly = false;
        }
        valid = spaceOnly;
    }
    //printf("line 60 %s\n", valid ? "true" : "false");
    spaceOnly = valid;
    while(counter < 11) {
        if (name[counter] != 32) spaceOnly = false;
        counter++;
    }
    valid = spaceOnly;
    return valid;
}

// checks if a character is of the legal value 0-9, a-z, A-Z
bool legalCharacter(char letter) {
    bool valid = false;
    //printf("letter value: %d\n", letter);
    if (letter > 47 && letter < 58) valid = true;
    if (letter > 64 && letter < 91) valid = true;
    if (letter > 96 && letter < 123) valid = true;
    return valid;
}

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
    for (int i = 0; i < 8; i++) {
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
        if (word[i] != 32) {
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
