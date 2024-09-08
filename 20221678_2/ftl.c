#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "flash.h"

// 필요한 경우 헤더파일을 추가한다

FILE *flashfp;  // fdevicedriver.c에서 사용

// 이 함수는 FTL의 역할 중 일부분을 수행하는데 물리적인 저장장치 flash memory에
// Flash device driver를 이용하여 데이터를 읽고 쓰거나 블록을 소거하는 일을
// 한다. flash memory에 데이터를 읽고 쓰거나 소거하기 위해서 fdevicedriver.c에서
// 제공하는 인터페이스를 호출하면 된다. 이때 해당되는 인터페이스를 호출할 때
// 연산의 단위를 정확히 사용해야 한다. 참고로, 읽기와 쓰기는 페이지 단위이며
// 소거는 블록 단위이다.

// 추가 주의사항!
// 빈블록을 남겨놔야하기 때문에 1번블록부터 시작해야하며 실질적으로 ppn=4부터
// 블록을 사용한다 봐야함 한 바이트만 읽으면 안된다 → 읽을때 쓸때 단위는 페이지
// 단위! 페이지 자체를 읽어 와서 거기의 첫번째 바이트를 읽어야 한다!

char *removeQuotes(const char *str) {
  int len = strlen(str);
  char *newStr = NULL;

  if (len > 2 && str[0] == '"' && str[len - 1] == '"') {
    newStr = (char *)malloc(len - 1);
    if (newStr == NULL) {
      fprintf(stderr, "Memory allocation failed\n");
      exit(1);
    }

    // 양 끝의 큰따옴표를 제외하고 문자열 복사
    strncpy(newStr, str + 1, len - 2);
    newStr[len - 2] = '\0';
  } else {
    newStr = strdup(str);
  }

  return newStr;
}

int main(int argc, char *argv[]) {
  // 페이지 단위로 읽기 위함
  char sectorbuf[SECTOR_SIZE];
  char sparebuf[SPARE_SIZE];
  char pagebuf[PAGE_SIZE];
  char blockbuf[BLOCK_SIZE];
  // 0번 블록으로 카피할떄 유효한 페이지만 카피하기 위함
  int availableArr[PAGE_NUM]={0,0,0,0};

  // 총페이지읽기 수, 총페이지쓰기수, 총블록소거수
  int pageRD = 0;
  int pageWR = 0;
  int blockER = 0;

  // flash memory 파일 생성: 위에서 선언한 flashfp를 사용하여 flash 파일을
  // 생성한다. 그 이유는 fdevicedriver.c에서
  //                 flashfp 파일포인터를 extern으로 선언하여 사용하기 때문이다.
  if (!strcmp(argv[1], "c")) {
    // a.out c <flashfile> <#blocks>
    int blocks = atoi(argv[3]);
    // 읽기+쓰기 아무튼 새 파일 생성
    flashfp = fopen(argv[2], "w+b");
    if (flashfp == NULL) {
      fprintf(stderr, "ERROR: file open error!\n");
      exit(1);
    }
    // 블록은 4개의 페이지로 구성
    // 각 페이지는 하나의 512B 섹터영역과 16B 스페어영역(spare area)으로 구성
    char data[BLOCK_SIZE * blocks];
    memset(data, 0xFF, BLOCK_SIZE);  // 데이터 초기화

    for (int i = 0; i < blocks; i++) {
      if (fwrite(data, 1, BLOCK_SIZE, flashfp) != BLOCK_SIZE) {
        fprintf(stderr, "ERROR: file write error!\n");
        fclose(flashfp);  // 파일을 닫고 프로그램 종료
        exit(1);
      }
    }

    fseek(flashfp, 0, SEEK_SET);
  } else if (!strcmp(argv[1], "w")) {
    // 페이지 쓰기: pagebuf의 sector 영역과 spare 영역에 각각 데이터를 정확히
    // 저장하고 난 후 해당 인터페이스를 호출한다. a.out w <flashfile> <ppn>
    // <sectordata> <sparedata>
    int ppn = atoi(argv[3]);
    // 0번째 블록은 항상 비워야하기 때문

    int blocks = ppn / PAGE_NUM;

    if (0 <= ppn && ppn <= 3) {
      fprintf(stderr, "ERROR : cannot write in the ppn 0 to ppn 3\n");
      exit(1);
    }

    char *sectorData = removeQuotes(argv[4]);

    char *spareData = removeQuotes(argv[5]);

    // 읽기 +쓰기 / 이미 파일이 존재해야
    flashfp = fopen(argv[2], "r+b");
    if (flashfp == NULL) {
      fprintf(stderr, "ERROR: file open error!\n");
      exit(1);
    }

    // 1) 해당 페이지에 데이터가 존재하는지 부터 확인 ppn*528
    // 존재하지 않는다면
    if (dd_read(ppn, pagebuf) < 0) {
      fprintf(stderr, "Error: #%d page read error\n", ppn);
      exit(1);
    }
    pageRD++;

    if ((unsigned char)pagebuf[0] == 0xFF) {
      // 해당 페이지에 데이터가 없는경우
      // 2) 만약 주어진 섹터데이터가 512B보다 적을 경우 나 머지 공간은 0xFF로
      // 채운다 abcd12345@%$를 16 번째 페이지에 처음부터 채우고 나 머지 500B는
      // 0xFF로 채운다.

      // 양끝에 큰따옴표가 있는지 확인

      memset(pagebuf, 0xFF, PAGE_SIZE);
      memcpy(pagebuf, sectorData, strlen(sectorData));
      memcpy(pagebuf + SECTOR_SIZE, spareData, strlen(spareData));
      if (dd_write(ppn, pagebuf) < 0) {
        fprintf(stderr, "Error: #%dpage write error\n", ppn);
        exit(1);
      }

      pageWR++;
    } else {
      // 해당 페이지에 데이터가 있는 경우

      // 해당 페이지가 속한 블록 번호를 찾아내기
      int pbn = ppn / PAGE_NUM;

      // overwrite될 인덱스 (ignore할것)
      int ignoreIdx = ppn % PAGE_NUM;

      // 2)해당 페이지가 속한 블록안의 데이터 내용을 읽는다. -> 블록 안의 페이지 4개 다 읽어봐야함
      memset(blockbuf, 0xFF, BLOCK_SIZE);
      for (int i = 0; i < PAGE_NUM; i++) {
        // overwrite될 인덱스는 읽지 않는다.
        if (i == ignoreIdx) {
          continue;
        }
        memset(pagebuf, 0xFF, PAGE_SIZE);
        
        if (dd_read(pbn * PAGE_NUM + i, pagebuf) < 0) {
          fprintf(stderr, "Error: block copy error\n");
          exit(1);
        }
        if((unsigned char)pagebuf[0]!=0xFF){
          availableArr[i]=1;
        }
        
        memcpy(blockbuf + i * PAGE_SIZE, pagebuf, PAGE_SIZE);
        pageRD++;
      }
      // 2-1)  0번 블록에 카피한다(쓴다) => 유효한 페이지만 쓴다.

    
    for (int i = 0; i < PAGE_NUM; i++) {
        memset(pagebuf, 0xFF, PAGE_SIZE);
        memcpy(pagebuf, blockbuf + i * PAGE_SIZE, PAGE_SIZE);
        // overwrite될 인덱스는 쓰지 않는다.
        if (i == ignoreIdx) {
          continue;
        }
        if(availableArr[i]==1){
          if (dd_write(i, pagebuf) < 0) {
            fprintf(stderr, "Error: block copy error\n");
            exit(1);
          }
          pageWR++;
        } 
      }

      // 3) 해당 페이지가 속한 블록 전부 erase처리 해버린다.
      if (dd_erase(pbn) < 0) {
        fprintf(stderr, "Error: #%d block erase error\n", pbn);
        exit(1);
      }
      blockER++;

      // 4) 입력으로 받아온 페이지에 데이터 쓴다.(섹터&스페어)

      memset(pagebuf, 0xFF, PAGE_SIZE);
      memcpy(pagebuf, sectorData, strlen(sectorData));
      memcpy(pagebuf + SECTOR_SIZE, spareData, strlen(spareData));
      if (dd_write(ppn, pagebuf) < 0) {
        fprintf(stderr, "Error: #%d page write error\n", ppn);
        exit(1);
      }

      pageWR++;

      // 5) 0번 블록에 있는애들 입력으로 받아온 페이지가 속한 블록에 recopy
      // 5-1) 0번 블록 자체를 읽는다 (overwrite된 인덱스 제외) => 이미 0번 블록에 카피하면서 유효한 페이지 확인했기에 recopy시에는 유효한 데이터만 읽음
      memset(blockbuf, 0xFF, BLOCK_SIZE);
      for (int i = 0; i < PAGE_NUM; i++) {
        // overwrite될 인덱스는 읽지 않는다.
        if (i == ignoreIdx) {
          continue;
        }
        if(availableArr[i]==1){
          if (dd_read(i, pagebuf) < 0) {
            fprintf(stderr, "Error: #0 block #%d page read error\n", ppn);
            exit(1);
          }
          memcpy(blockbuf + i * PAGE_SIZE, pagebuf, PAGE_SIZE);
          pageRD++;
        }
      }
      // 5-2) 0번 블록에서 읽어온 데이터를 원래 카피해온 블록에 다시 쓴다 => 유효한 페이지만 쓴다.
      int startPPN = pbn * PAGE_NUM;

      for (int i = 0; i < PAGE_NUM; i++) {
        if (i == ignoreIdx) {
          continue;
        }
        if(availableArr[i]==1){

          if (dd_write(startPPN + i, blockbuf + i * PAGE_SIZE) < 0) {
            fprintf(stderr, "Error: #%d page write error\n", ppn);
            exit(1);
          }
          pageWR++;
        }
        
      }

      // 6) 0번 블록도 erase시킨다.
      if (dd_erase(0) < 0) {
        fprintf(stderr, "Error: #0 block erase error\n");
        exit(1);
      }
      blockER++;
    }

    // 페이지 쓰기를 수행하기 위한 총페이지읽기 수, 총페이지쓰기수,
    // 총블록소거수를 출력한다
    printf("#pagereads=%d #pagewrites=%d #blockerases=%d\n", pageRD, pageWR,
           blockER);
  } else if (!strcmp(argv[1], "r")) {
    // 페이지 읽기: pagebuf를 인자로 사용하여 해당 인터페이스를 호출하여
    // 페이지를 읽어 온 후 여기서 sector 데이터와
    //                  spare 데이터를 분리해 낸다
    // a.out r <flashfile> <ppn>
    int ppn = atoi(argv[3]);
    if (0 <= ppn && ppn <= 3) {
      fprintf(stderr, "ERROR: cannot read ppn 0 to ppn 3\n");
      exit(1);
    }
    // 읽기모드로만 열기
    flashfp = fopen(argv[2], "rb");
    if (flashfp == NULL) {
      fprintf(stderr, "ERROR: file open error!\n");
      exit(1);
    }
    memset(pagebuf, 0xFF, PAGE_SIZE);
    if (dd_read(ppn, pagebuf) < 0) {
      fprintf(stderr, "Error: #%d page read error\n", ppn);
      exit(1);
    }
    pageRD++;
    // 섹터 데이터만 분리
    memset(sectorbuf, 0xFF, SECTOR_SIZE);
    memset(sparebuf, 0xFF, SPARE_SIZE);
    memcpy(sectorbuf, pagebuf, SECTOR_SIZE);
    // 스페어 데이터만 분리
    memcpy(sparebuf, pagebuf + SECTOR_SIZE, SPARE_SIZE);

    int nullFlag = 1;
    for (int i = 0; i < SECTOR_SIZE; i++) {
      if ((unsigned char)sectorbuf[i] != 0xFF) {
        nullFlag = 0;
      }
    }
    for (int i = 0; i < SPARE_SIZE; i++) {
      if ((unsigned char)sparebuf[i] != 0xFF) {
        nullFlag = 0;
      }
    }
    if (nullFlag == 1) {
    } else {
      for (int i = 0; i < SECTOR_SIZE; i++) {
        if ((unsigned char)sectorbuf[i] != 0xFF) {
          printf("%c", sectorbuf[i]);
        }
      }
      printf(" ");  // 섹터 데이터와 스페어 데이터 사이에 공백을 추가

      // sparebuf에서 0xFF가 아닌 바이트만 출력
      for (int i = 0; i < SPARE_SIZE; i++) {
        if ((unsigned char)sparebuf[i] != 0xFF) {
          printf("%c", sparebuf[i]);
        }
      }
      printf("\n");
    }
  } else if (!strcmp(argv[1], "e")) {
    // a.out e <flashfile> <pbn>
    flashfp = fopen(argv[2], "r+b");
    if (flashfp == NULL) {
      fprintf(stderr, "ERROR: file open error!\n");
      exit(1);
    }

    int pbn = atoi(argv[3]);

    if (pbn <= 0) {
      fprintf(stderr, "ERROR : cannot erase pbn 0\n");
      exit(1);
    }
    if (dd_erase(pbn) < 0) {
      fprintf(stderr, "Error: #%d block erase error\n", pbn);
      exit(1);
    }
    blockER++;
  }

  return 0;
}
