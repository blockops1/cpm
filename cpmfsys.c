#include "cpmfsys.h"
#include <ctype.h>

#define NUM_EXTENTS 16

// internal function, returns -1 for illegal name or name not found
// otherwise returns extent nunber 0-31
int findExtentWithName(char *name, uint8_t *block0);

// populate a firstName and an extName from a fullName
int splitOutName(char *firstName, char *extName, char *fullName);

// populate a fullName from a firstName and an extName
int combineName(char *fullName, char *firstName, char *extName);

// checks for legal characters of 0-9, A-Z, a-z
bool legalCharacter(char);

// calculate and return filesize from the directory entry
int fileSize(DirStructType *);

int trimString(char *);

//normalize any legal input name to 8.3 by padding with spaces
int normalizeName(char *inputName, char *outputName);

//bool freeList[BLOCK_SIZE/EXTENT_SIZE * BLOCKS_PER_EXTENT];
bool freeList[255];

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
    for (int i = 0; i < 32; i++)
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
    //printf("Name: %s\n", nameTest);
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
    e[line + 13] = d->BC;
    e[line + 15] = d->RC;
    for (int i = 0; i < 8; i++)
    {
        e[line + 1 + i] = d->name[i];
    }
    for (int i = 0; i < 3; i++)
    {
        e[line + 9 + i] = d->extension[i];
    }
    for (int i = 0; i < 32; i++)
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

    //DirStructType entry = {0};
    DirStructType *entry_p = NULL;
    //entry_p = &entry;
    // first mark all blocks true
    for (int i = 0; i < BLOCK_SIZE; i++)
    {
        freeList[i] = true;
    }
    //printf("zeroed freelist\n");
    // go through each directory entry
    for (int i = 0; i < NUM_EXTENTS; i++)
    {
        //printf("entry %d\n", i);
        entry_p = mkDirStruct(i, buffer_p);
        //fillExtent(entry_p, buffer_p, i);
        //line_p = mkDirStruct(i, buffer_p);
        //printf("entry %d\n", i);
        if (entry_p->status >= 0 && entry_p->status <= 15)
        {
            for (int j = 0; j < BLOCKS_PER_EXTENT; j++)
            {
                //printf("%x ", i * NUM_EXTENTS + j);
                if (entry_p->blocks[j] != 0x00)
                    freeList[i * NUM_EXTENTS + j] = false;
            }
        }
        //printf("\n");
        free(entry_p);
    }
    return;
}

// debugging function, print out the contents of the free list in 16 rows of 16, with each
// row prefixed by the 2-digit hex address of the first block in that row. Denote a used
// block with a *, a free block with a .
void printFreeList()
{
    makeFreeList();
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
        //fillExtent(entry_p, block0, line);
        //printf("after fill line: %d firstname: %s ext: %s\n", line, firstName, extName);
        //printf("after fill line2: %d firstname: %s ext: %s\n", line, entry_p->name, entry_p->extension);
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
    // split out the provided name into first 8 and 3 ext
    //printf("split fullname %s\n", fullName);
    //printf("split lenght firstname: %lu\n", strlen(firstName));
    //printf("split lenght ext name: %lu\n", strlen(extName));
    if (strlen(firstName) != 8 || strlen(extName) != 3)
    {
        printf("stringlength problem\n");
        return -1;
    }
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

// populate a fullName from a firstName and an extName
int combineName(char *fullName, char *firstName, char *extName)
{
    // split out the provided name into first 8 and 3 ext
    if (strlen(fullName) != 12)
        return -1;
    if (strlen(firstName) != 8 || strlen(extName) != 3)
        return -1;
    for (int i = 0; i < 8; i++)
    {
        fullName[i] = firstName[i];
    }
    fullName[8] = 46;
    for (int i = 0; i < 4; i++)
    {
        fullName[i + 9] = extName[i];
    }
    if (!checkLegalName(fullName))
        return -1;
    return 0;
}

// internal function, returns true for legal name (8.3 format), false for illegal
// (name or extension too long, name blank, or  illegal characters in name or extension)
// make sure name is normalized first. If not, use normalizeName()
bool checkLegalName(char *name)
{
    bool valid = true;
    //printf("%s\n", name);
    if (name == NULL)
        return false;
    // normalize the input
    char name2[13] = "";
    normalizeName(name, name2);
    //if (strlen(name) != 12)
    //    return false;
    //if (name[8] != *".")
    //    return false;
    // check first character 0-9, a-z, A-Z
    valid = legalCharacter(name2[0]);
    //check other characters 0-9, a-z, A-Z. If a space, rest of name and ext needs to be space
    int counter = 0;
    while (valid && counter < 7)
    {
        counter++;
        valid = legalCharacter(name2[counter]);
    }
    //printf("line 37 %s\n", valid ? "true" : "false");
    if (counter < 8 && name2[counter] == 32)
    {
        valid = true;
        counter++;
    }
    bool spaceOnly = valid;
    //printf("counter: %d\n", counter);
    //printf("line 40 %s\n", valid ? "true" : "false");
    while (counter < 7)
    {
        counter++;
        if (name2[counter] != 32)
            spaceOnly = false;
    }
    valid = spaceOnly;
    //printf("counter: %d\n", counter);
    // counter must be 7 by here, now we check the extension
    counter++;
    //printf("line 48 %s\n", valid ? "true" : "false");
    //printf("counter: %d\n", counter);
    while (valid && counter < 11)
    {
        counter++;
        valid = legalCharacter(name2[counter]);
    }
    //printf("line 54 %s\n", valid ? "true" : "false");
    if (counter < 12 && name2[counter] == 32)
        valid = true;
    spaceOnly = valid;
    if (counter == 9 && name2[counter] == 32)
    {
        spaceOnly = true;
        while (spaceOnly && counter < 11)
        {
            counter++;
            if (name2[counter] != 32)
                spaceOnly = false;
        }
        valid = spaceOnly;
    }
    //printf("line 60 %s\n", valid ? "true" : "false");
    spaceOnly = valid;
    while (counter < 11)
    {
        if (name2[counter] != 32)
            spaceOnly = false;
        counter++;
    }
    valid = spaceOnly;
    return valid;
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
    //DirStructType entry = {0};
    DirStructType *entry_p = NULL;
    //entry_p = &entry;
    for (line = 0; line < NUM_EXTENTS; line++)
    {
        //printf("line: %d\n", line);
        // this section prints the directory entry
        if (buffer[line * EXTENT_SIZE] != 0xe5)
        {
            //fillExtent(entry_p, buffer_p, line);
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
        //zero out the structure
        //memset(entry_p, 0, sizeof(*entry_p));
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
    //char fullName[13];
    splitOutName(extent->name, extent->extension, newName2);
    writeDirStruct(extent, line, buffer_p);
    free(extent);
    return blockWrite(buffer_p, 0);
}

//normalize any legal input name to 8.3 by padding with spaces. outputname must be char[13]
int normalizeName(char *inputName, char *outputName)
{
    size_t outputName_size = sizeof(outputName)/sizeof(outputName[0]);
    //size_t outputName_size = sizeof(outputName);
    //printf("output name array size: %lu\n", outputName_size);
    if (outputName_size < 8)
    {
        printf("outputName too small\n");
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
    outCounter++;
    inCounter++;
    while (inputName[inCounter] != 46 && inputName[inCounter] != 0 && inCounter < 12)
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
    //printf("inputName %s\n", inputName);
    //printf("OutputName %s\n", outputName);
    return 0;
}

// delete the file named name, and free its disk blocks in the free list
int cpmDelete(char *name)
{
    return 0;
}

// following functions need not be implemented for Lab 2

int cpmCopy(char *oldName, char *newName)
{
    return 0;
}

int cpmOpen(char *fileName, char mode)
{
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
