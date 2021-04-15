#include <stdio.h>
#include <stdint.h>
#include "cpmfsys.h"
#include "diskSimulator.h"


// for debugging, prints a region of memory starting at buffer with
void printBuffer(uint8_t buffer[], int size)
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

int main(int argc, char *argv[])
{
  //uint8_t buffer1[BLOCK_SIZE],buffer2[BLOCK_SIZE];
  //int i;
  readImage("image1.img");
  makeFreeList();
  printFreeList();
  cpmDir();
  char testname[13] = "testfil1.std";
  printf("%s: ", testname);
  printf("%s\n", checkLegalName(testname) ? "true" : "false");
  strcpy(testname, "testfil1. a ");
  printf("%s: ", testname);
  printf("%s\n", checkLegalName(testname) ? "true" : "false");
  strcpy(testname, "testfil1.   ");
  printf("%s: ", testname);
  printf("%s\n", checkLegalName(testname) ? "true" : "false");
  strcpy(testname, "testfil1.a  ");
  printf("%s: ", testname);
  printf("%s\n", checkLegalName(testname) ? "true" : "false");
  strcpy(testname, "test    .bbb");
  printf("%s: ", testname);
  printf("%s\n", checkLegalName(testname) ? "true" : "false");
  strcpy(testname, "test   s.bbb");
  printf("%s: ", testname);
  printf("%s\n", checkLegalName(testname) ? "true" : "false");
  strcpy(testname, "testdds1.  b");
  printf("%s: ", testname);
  printf("%s\n", checkLegalName(testname) ? "true" : "false");
strcpy(testname, "testdd  .   ");
printf("%s: ", testname);
printf("%s\n", checkLegalName(testname) ? "true" : "false");
strcpy(testname, "gob");
printf("%s: ", testname);
printf("%s\n", checkLegalName(testname) ? "true" : "false");
  DirStructType entry = {0};
  DirStructType *entry_p = NULL;
  entry_p = &entry;
  //go through the block to fill the entry
  entry.status = 0x01;
  entry.XL = 0x00;
  entry.BC = 0x01;
  entry.XH = 0x00;
  entry.RC = 0x02;
  strcpy(entry.name, "testone1");
  strcpy(entry.extension, "txt");
  entry.blocks[0] = 0x52;
  uint8_t buffer1[BLOCK_SIZE] = {0};
  uint8_t buffer2[BLOCK_SIZE] = {0};
  uint8_t *buffer1_p = NULL;
  uint8_t *buffer2_p = NULL;
  buffer1_p = (uint8_t *)&buffer1;
  buffer2_p = (uint8_t *)&buffer2;
  blockRead(buffer1_p, 0);
  blockRead(buffer2_p, 0);
  writeDirStruct(entry_p, 2, buffer2_p);
  cpmDir();
  printBuffer(buffer1, 128);
  printBuffer(buffer2, 128);
  cpmRename("mygotoad.txt", "mygo.txt");
  cpmDir();
  cpmRename("mytestf2.new", "gob");
  cpmDir();
  printFreeList();
  cpmDelete("shortf.ps");
  cpmDir();
  cpmRename("mytestf1.txt","mytest2.tx");
  fprintf(stdout,"cpmRename return code = %d,\n",cpmRename("mytestf","mytestv2.x"));
  cpmDir();
  printFreeList();
  printf("doing cpmCopy(mytest2.tx, magootwo.txt); \n");
  blockRead(buffer1_p, 0);
  printBuffer(buffer1, 128);
  int result = cpmCopy("mytest2.tx", "magootwo.txt");
  printf("result of copy: %d\n", result);
  blockRead(buffer1_p, 0);
  printBuffer(buffer1, 128);
  cpmDir();
  printFreeList();
  //sleep(10);
}
