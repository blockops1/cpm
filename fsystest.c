#include <stdio.h> 
#include <stdint.h> 
#include "cpmfsys.h" 
#include "diskSimulator.h"

// for debugging, prints a region of memory starting at buffer with 
void printBuffer(uint8_t buffer[],int size) { 
  int i;
  fprintf(stdout,"\nBUFFER PRINT:\n"); 
  for (i = 0; i < size; i++) { 
    if (i % 16 == 0) { 
      fprintf(stdout,"%4x: ",i); 
    }
    fprintf(stdout, "%2x ",buffer[i]);
    if (i % 16 == 15) { 
      fprintf(stdout,"\n"); 
    }
  }
  fprintf(stdout,"\n"); 
} 


int main(int argc, char * argv[]) { 
  uint8_t buffer1[BLOCK_SIZE],buffer2[BLOCK_SIZE]; 
  int i; 
  readImage("image1.img"); 
  //makeFreeList(); 
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

    uint8_t buffer[1024] = {0};
    uint8_t *buffer_p = NULL;
    buffer_p = (uint8_t * ) & buffer;
    blockRead(buffer_p, 0);
    int testExtent[32] = {0x01, 0x6d, 0x6f, 0x73, 0x62, 0x73, 0x74, 0x66, 0x31, 0x74, 0x78, 0x74, 0x01, 0x02, 0x03, 0x04, 0xff, 0x0e, 0xfd, 0x0c, 0xfb, 0x0a, 0xf9, 0x08, 0xf7, 0x06, 0xf5, 0x04, 0xf3, 0x02, 0xf1, 0x10};
    DirStructType *testExtent_p = NULL;
    testExtent_p = (DirStructType *)&testExtent;
    writeDirStruct(testExtent_p, 2, buffer_p);
  //printFreeList();
  //cpmDelete("shortf.ps");
  //cpmDir();
  //cpmRename("mytestf1.txt","mytest2.tx");
  //fprintf(stdout,"cpmRename return code = %d,\n",cpmRename("mytestf","mytestv2.x")); 
  //cpmDir(); 
  //printFreeList(); 
}

