#include "cpmfsys.h"
#include <ctype.h>

#define NUM_EXTENTS 16

// internal function, returns -1 for illegal name or name not found
// otherwise returns extent nunber 0-31
int findExtentWithName(char *name, uint8_t *block0);

// populate a firstName and an extName from a fullName
int splitOutName(char *firstName, char *extName, char *fullName);

// checks for legal characters of 0-9, A-Z, a-z
bool legalCharacter(char);

// calculate and return filesize from the directory entry
int fileSize(DirStructType *);

// remove the spaces from the end of a string
int trimString(char *);

//normalize any legal input name to 8.3 by padding with spaces
int normalizeName(char *inputName, char *outputName);

// debugging function to print out a buffer
void printBuffer2(uint8_t buffer[], int size);

bool freeList[NUM_BLOCKS];

//function to allocate memory for a DirStructType (see above), and populate it, given a
//pointer to a buffer of memory holding the contents of disk block 0 (e), and an integer index
// which tells which extent from block zero (extent numbers start with 0) to use to make the
// DirStructType value to return.
DirStructType *mkDirStruct(int index, uint8_t *e)
{
    int line = index * EXTENT_SIZE;
    DirStructType *entry_p = (DirStructType *)malloc(sizeof(DirStructType));
    //DirStructType *entry_p = NULL;
    //entry_p = &entry;
    //go through the block to fill the entry
    entry_p->status = e[line];
    entry_p->XL = e[line + 12];
    entry_p->BC = e[line + 13];
    entry_p->XH = e[line + 14];
    entry_p->RC = e[line + 15];
    //printf("singles done\n");
    for (int i = 0; i < 8; i++)
    {
        entry_p->name[i] = e[line + 1 + i];
    }
    for (int i = 0; i < 3; i++)
    {
        entry_p->extension[i] = e[line + 9 + i];
    }
    for (int i = 0; i < BLOCKS_PER_EXTENT; i++)
    {
        entry_p->blocks[i] = e[line + 16 + i];
    }
    //printf("names done\n");
    return entry_p;
}

// function to write contents of a DirStructType struct back to the specified index of the extent
// in block of memory (disk block 0) pointed to by e
void writeDirStruct(DirStructType *d, uint8_t index, uint8_t *e)
{
    // check the values before writing to the block
    // check status between 0 and 15
    if (d->status < 0 || d->status > 15)
        return;
    // check BC
    if (d->BC < 0 || d->BC > 127)
        return;
    // check RC
    if (d->RC < 0 || d->RC > 7)
        return;
    // test name
    char nameTest[13];
    for (int i = 0; i < 8; i++)
    {
        nameTest[i] = d->name[i];
    }
    nameTest[8] = 46;
    for (int i = 0; i < 3; i++)
    {
        nameTest[i + 9] = d->extension[i];
    }
    //printf("NameTest: %s\n", nameTest);
    if (!checkLegalName(nameTest))
    {
        printf("checkLegalNAme failed\n");
        return;
    }
    // test valid blocks
    for (int i = 0; i < 32; i++)
    {
        if (d->blocks[i] < 0 || d->blocks[i] > 0xFF)
        {
            printf("illegal block number\n");
            return;
        }
    }
    // passed all checks, can write to block 0;
    int line = index * EXTENT_SIZE;
    e[line] = d->status;
    e[line + 12] = d->XL;
    e[line + 13] = d->BC;
    e[line + 14] = d->XH;
    e[line + 15] = d->RC;
    for (int i = 0; i < 8; i++)
    {
        e[line + 1 + i] = d->name[i];
    }
    for (int i = 0; i < 3; i++)
    {
        e[line + 9 + i] = d->extension[i];
    }
    for (int i = 0; i < 16; i++)
    {
        e[line + 16 + i] = d->blocks[i];
    }
    return;
}

// populate the FreeList global data structure. freeList[i] == true means
// that block i of the disk is free. block zero is never free, since it holds
// the directory. freeList[i] == false means the block is in use.
void makeFreeList()
{
    // go through each directory entry. If it is in use, check the blocks and mark them used
    uint8_t buffer[BLOCK_SIZE] = {0};
    uint8_t *buffer_p = NULL;
    buffer_p = (uint8_t *)&buffer;
    blockRead(buffer_p, 0);
    DirStructType *entry_p = NULL;
    // first mark all blocks true
    for (int i = 0; i < NUM_BLOCKS; i++)
    {
        freeList[i] = true;
    }
    //printf("zeroed freelist\n");
    // go through each directory entry
    for (int i = 0; i < NUM_EXTENTS; i++)
    {
        //printf("entry %d\n", i);
        entry_p = mkDirStruct(i, buffer_p);
        //printf("entry %d\n", i);
        if (entry_p->status >= 0 && entry_p->status <= 15)
        {
            //printf("filename: %s.%s", entry_p->name, entry_p->extension);
            for (int j = 0; j < BLOCKS_PER_EXTENT; j++)
            {
                if (entry_p->blocks[j] != 0x00)
                {
                    freeList[entry_p->blocks[j]] = false;
                }
            }
        }
        //printf("\n");
        free(entry_p);
    }
    freeList[0] = false;
    return;
}

// debugging function, print out the contents of the free list in 16 rows of 16, with each
// row prefixed by the 2-digit hex address of the first block in that row. Denote a used
// block with a *, a free block with a .
void printFreeList()
{
    //printf("call makefreelist\n");
    makeFreeList();
    //printf("completed makefreelist\n");
    printf("FREE BLOCK LIST: (* means in-use)\n ");
    for (int i = 0; i < NUM_EXTENTS; i++)
    {
        printf(" %x: ", i * BLOCKS_PER_EXTENT);
        for (int j = 0; j < BLOCKS_PER_EXTENT; j++)
        {
            if (freeList[i * NUM_EXTENTS + j])
            {
                printf(". ");
            }
            else
            {
                printf("* ");
            }
        }
        printf("\n");
    }
    return;
}

// internal function, returns -1 for illegal name or name not found
// otherwise returns extent number 0-31
int findExtentWithName(char *name, uint8_t *block0)
{
    //printf("findExtentWithName start\n");
    //printf("find - name to check: %s\n", name);
    if (!checkLegalName(name))
    {
        printf("find: not a legal name\n");
        return -1;
    }
    char firstName[9] = "testfile";
    char extName[4] = "txt";
    //DirStructType entry = {0};
    DirStructType *entry_p = NULL;
    //entry_p = &entry;
    // split out the provided name into first 8 and 3 ext
    if (splitOutName(firstName, extName, name) != 0)
    {
        printf("problem splitting name\n");
        return -1;
    }
    //printf("find: first: %s ext: %s full: %s\n", firstName, extName, name);
    //compare to names in each of the extents
    for (int line = 0; line < NUM_EXTENTS; line++)
    {
        //printf("before fill line: %d firstname: %s ext: %s\n", line, firstName, extName);
        entry_p = mkDirStruct(line, block0);
        if (strcmp(firstName, entry_p->name) == 0 && strcmp(entry_p->extension, extName) == 0)
        {
            //printf("found a match! line: %d firstname: %s ext: %s\n", line, firstName, extName);
            free(entry_p);
            return line;
        }
        else
        {
            free(entry_p);
        }
    }
    return -1;
}

// populate a firstName and an extName from a fullName
int splitOutName(char *firstName, char *extName, char *fullName)
{
    if (!checkLegalName(fullName))
    {
        printf("split: illegal name\n");
        return -1;
    }
    for (int i = 0; i < 8; i++)
    {
        firstName[i] = fullName[i];
    }
    //printf("firstname: %s\n", firstName);
    for (int i = 0; i < 4; i++)
    {
        extName[i] = fullName[i + 9];
    }
    //printf("extname: %s\n", extName);
    return 0;
}

// internal function, returns true for legal name (8.3 format), false for illegal
// (name or extension too long, name blank, or  illegal characters in name or extension)
// make sure name is normalized first. If not, use normalizeName()
bool checkLegalName(char *name)
{
    if (name == NULL)
        return false;
    char name2[13] = "";
    //change the input to a normalized 8.3
    //printf("check - name: %s\n", name);
    normalizeName(name, name2);
    //printf("check - name2: %s\n", name2);
    // we now have a single string with the 8.3 in correct place
    if (!legalCharacter(name2[0]))
        return false;
    bool firstSpace = false;
    int i = 1;
    //check first 8 valid
    while (i < 8)
    {
        //printf("check - name2 %s first 8 character is %c: %d\n", name2, name2[i], i);
        if (!((legalCharacter(name2[i])) || name2[i] == 32))
            return false;
        if (firstSpace && name2[i] != 32)
            return false;
        if (name2[i] == 32)
            firstSpace = true;
        i++;
    }
    //now check the 3 at the end. all spaces is permitted
    firstSpace = false;
    i = 9;
    while (i < 12)
    {
        if (!((legalCharacter(name2[i])) || name2[i] == 32))
            return false;
        if (firstSpace && name2[i] != 32)
            return false;
        if (name2[i] == 32)
            firstSpace = true;
        i++;
    }
    return true;
}

// checks if a character is of the legal value 0-9, a-z, A-Z
bool legalCharacter(char letter)
{
    bool valid = false;
    //printf("letter value: %d\n", letter);
    if (letter > 47 && letter < 58)
        valid = true;
    if (letter > 64 && letter < 91)
        valid = true;
    if (letter > 96 && letter < 123)
        valid = true;
    return valid;
}

// print the file directory to stdout. Each filename should be printed on its own line,
// with the file size, in base 10, following the name and extension, with one space between
// the extension and the size. If a file does not have an extension it is acceptable to print
// the dot anyway, e.g. "myfile. 234" would indicate a file whose name was myfile, with no
// extension and a size of 234 bytes. This function returns no error codes, since it should
// never fail unless something is seriously wrong with the disk
void cpmDir()
{
    printf("DIRECTORY LISTING\n");
    uint8_t buffer[BLOCK_SIZE] = {0};
    uint8_t *buffer_p = NULL;
    buffer_p = (uint8_t *)&buffer;
    blockRead(buffer_p, 0);
    int line = 0;
    char trimName[9];
    char trimExt[4];
    int length = 0;
    DirStructType *entry_p = NULL;
    for (line = 0; line < NUM_EXTENTS; line++)
    {
        //printf("line: %d\n", line);
        // this section prints the directory entry
        if (buffer[line * EXTENT_SIZE] != 0xe5)
        {
            entry_p = mkDirStruct(line, buffer_p);
            strcpy(trimName, entry_p->name);
            strcpy(trimExt, entry_p->extension);
            length = trimString(trimName);
            //printf("length: %d", length);
            printf("%.*s.", length, trimName);
            length = trimString(trimExt);
            //printf("length: %d\n", length);
            printf("%.*s ", length, trimExt);
            printf("%d\n", fileSize(entry_p));
            memset(trimName, 0, sizeof(trimName));
            memset(trimExt, 0, sizeof(trimExt));
            free(entry_p);
        }
    }
    return;
}

// calculate and return the filesize of a directory entry
int fileSize(DirStructType *entry)
{
    // count the number of full blocks
    int blockNumber = 0;
    int size = 0;
    while (blockNumber < 16)
    {
        if (entry->blocks[blockNumber] != 0)
        {
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
int trimString(char *word)
{
    int length = 0;
    int size = strlen(word);
    char word2[size];
    for (int i = 0; i < size; i++)
    {
        if (word[i] != 32)
        {
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
int cpmRename(char *oldName, char *newName)
{
    //printf("oldname: %s newname: %s\n", oldName, newName);
    int line = -1;
    uint8_t buffer[BLOCK_SIZE] = {0};
    uint8_t *buffer_p = NULL;
    buffer_p = (uint8_t *)&buffer;
    blockRead(buffer_p, 0);
    //printf("looking for newName %s\n", newName);
    //need to normalize newName to 8.3 format from whatever is typed
    char oldName2[13] = "";
    char newName2[13] = "";
    normalizeName(oldName, oldName2);
    normalizeName(newName, newName2);
    //printf("looking for newName %s\n", newName2);
    if (!checkLegalName(newName2))
    {
        printf("cpm - illegal new name\n");
        return -2;
    }
    line = findExtentWithName(newName2, buffer_p);
    if (line != -1)
    {
        printf("new name already in directory\n");
        return -3;
    }
    //printf("looking for oldName %s\n", oldName2);
    line = findExtentWithName(oldName2, buffer_p);
    if (line == -1)
    {
        printf("old name not found\n");
        return -2;
    }
    DirStructType *extent = mkDirStruct(line, buffer_p);
    splitOutName(extent->name, extent->extension, newName2);
    //printf("newname2: %s\n", newName2);
    writeDirStruct(extent, line, buffer_p);
    free(extent);
    return blockWrite(buffer_p, 0);
}

//normalize any legal input name to 8.3 by padding with spaces. outputname must be char[13]
int normalizeName(char *inputName, char *outputName)
{
    size_t outputName_size = sizeof(outputName) / sizeof(outputName[0]);
    //printf("output name array size: %lu\n", outputName_size);
    if (outputName_size < 8)
    {
        //printf("outputName too small\n");
        return -1;
    }
    int inCounter = 0;
    int outCounter = 0;
    while (inputName[inCounter] != 46 && inputName[inCounter] != 0 && inCounter < 8)
    {
        outputName[outCounter] = inputName[inCounter];
        inCounter++;
        outCounter++;
    }
    while (outCounter < 8)
    {
        outputName[outCounter] = 32;
        outCounter++;
    }
    outputName[8] = 46;
    // if we have hit an end of string, fill rest of outputNane with spaces
    if (inputName[inCounter] == 0)
    {
        for (int i = 9; i < 12; i++)
        {
            outputName[i] = 32;
        }
        return 0;
    }
    outCounter++;
    inCounter++;
    while (inputName[inCounter] != 32 && inputName[inCounter] != 0 && inCounter < 12)
    {
        outputName[outCounter] = inputName[inCounter];
        inCounter++;
        outCounter++;
    }
    while (outCounter < 12)
    {
        outputName[outCounter] = 32;
        outCounter++;
    }
    return 0;
}

// delete the file named name, and free its disk blocks in the free list
int cpmDelete(char *name)
{
    // check input name
    //need to normalize newName to 8.3 format from whatever is typed
    char name2[13] = "";
    normalizeName(name, name2);
    if (!checkLegalName(name2))
    {
        //printf("cpm - illegal filename\n");
        return -2;
    }
    //printf("cpmdelete name2: %s\n", name2);
    // get block 0
    int line = -1;
    uint8_t buffer[BLOCK_SIZE] = {0};
    uint8_t *buffer_p = NULL;
    buffer_p = (uint8_t *)&buffer;
    blockRead(buffer_p, 0);
    // find the line
    line = findExtentWithName(name2, buffer_p);
    //printf("cpmdelete found name: %s at line %d\n", name2, line);
    if (line == -1)
    {
        //printf("name not in directory\n");
        return -1;
    }
    //write directly to the fricking block
    buffer_p[line * EXTENT_SIZE] = 0xe5;
    for (int i = 1; i < EXTENT_SIZE; i++)
    {
        buffer_p[line * EXTENT_SIZE + i] = 0x00;
    }
    // write to block 0
    blockWrite(buffer_p, 0);
    return 0;
}

// for debugging, prints a region of memory starting at buffer with
void printBuffer2(uint8_t buffer[], int size)
{
    int i;
    fprintf(stdout, "\nBUFFER PRINT:\n");
    for (i = 0; i < size; i++)
    {
        if (i % 16 == 0)
        {
            fprintf(stdout, "%4x: ", i);
        }
        fprintf(stdout, "%2x ", buffer[i]);
        if (i % 16 == 15)
        {
            fprintf(stdout, "\n");
        }
    }
    fprintf(stdout, "\n");
}

// following functions need not be implemented for Lab 2

int cpmCopy(char *oldName, char *newName)
{
    // read oldName, count number of blocks used
    if (oldName == NULL || !checkLegalName(oldName))
        return -2;
    if (newName == NULL || !checkLegalName(newName))
        return -2;
    // get block 0
    int oldLine = -1;
    int newLine = -1;
    uint8_t *buffer_p = (uint8_t *)(malloc(BLOCK_SIZE * sizeof(uint8_t)));
    blockRead(buffer_p, 0);
    //printf("finished getblock0\n");
    // check if newname already in use, if not, find an unused line
    char newName2[13] = "";
    normalizeName(newName, newName2);
    newLine = findExtentWithName(newName2, buffer_p);
    if (newLine != -1)
        return -3;
    bool openLine = false;
    while (newLine < 16 && !openLine)
    {
        newLine++;
        if (buffer_p[newLine * EXTENT_SIZE] == 0xe5)
            openLine = true;
    }
    if (!openLine)
        return -4;
    // find the oldname line
    char oldName2[13] = "";
    normalizeName(oldName, oldName2);
    oldLine = findExtentWithName(oldName2, buffer_p);
    if (oldLine == -1)
    {
        printf("can't find oldName %s\n", oldName);
        return -1;
    }
    // create an extent from the oldLine number
    DirStructType *oldEntry_p = mkDirStruct(oldLine, buffer_p);
    // count the number of blocks
    int blockNumber = 0;
    int blockCount = 0;
    while (blockNumber < 16)
    {
        if (oldEntry_p->blocks[blockNumber] != 0)
        {
            blockCount++;
        }
        blockNumber++;
    }
    // check freeblock list for how many blocks free
    makeFreeList();
    int freeBlockCount = 0;
    for (int i = 0; i < NUM_BLOCKS; i++)
    {
        if (freeList[i])
            freeBlockCount++;
    }
    if (freeBlockCount < blockCount)
        return -4;
    // create an extent for newName using available blocks
    DirStructType *newEntry_p = (DirStructType *)malloc(sizeof(DirStructType));
    splitOutName(newEntry_p->name, newEntry_p->extension, newName2);
    // copy all info from old to new except blocks
    newEntry_p->status = oldEntry_p->status;
    newEntry_p->XL = oldEntry_p->XL;
    newEntry_p->BC = oldEntry_p->BC;
    newEntry_p->XH = oldEntry_p->XH;
    newEntry_p->RC = oldEntry_p->RC;
    // create a new blocklist in the new extent
    int freeBlock = 0;
    int newBlock = 0;
    while (freeBlock < NUM_BLOCKS && newBlock < blockCount)
    {
        if (freeList[freeBlock])
        {
            newEntry_p->blocks[newBlock] = freeBlock;
            newBlock++;
        }
        freeBlock++;
    }
    // copy block data from old to new
    uint8_t *dataBuffer_p = (uint8_t *)(malloc(BLOCK_SIZE * sizeof(uint8_t)));
    while (blockNumber < 16)
    {
        if (oldEntry_p->blocks[blockNumber] != 0x00)
        {
            blockRead(dataBuffer_p, oldEntry_p->blocks[blockNumber]);
            blockWrite(dataBuffer_p, newEntry_p->blocks[blockNumber]);
        }
        blockNumber++;
    }
    // write new extent to newLine
    writeDirStruct(newEntry_p, newLine, buffer_p);
    blockWrite(buffer_p, 0);
    // find
    free(oldEntry_p);
    free(newEntry_p);
    free(buffer_p);
    free(dataBuffer_p);
    return 0;
}

int cpmOpen(char *fileName, char mode)
{
    // load the extent and block list
    // have a pointer, return pointer number?
    // pointer needs to be a global variable
    return 0;
}

// non-zero return indicates filePointer did not point to open file
int cpmClose(int filePointer)
{
    return 0;
}

// returns number of bytes read, 0 = error
int cpmRead(int pointer, uint8_t *buffer, int size)
{
    return 0;
}

// returns number of bytes written, 0 = error
int cpmWrite(int pointer, uint8_t *buffer, int size)
{
    return 0;
}
