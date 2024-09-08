// 주의사항
// 1. blockmapping.h에 정의되어 있는 상수 변수를 우선적으로 사용해야 함
// 2. blockmapping.h에 정의되어 있지 않을 경우 본인이 이 파일에서 만들어서 사용하면 됨
// 3. 필요한 data structure가 필요하면 이 파일에서 정의해서 쓰기 바람(blockmapping.h에 추가하면 안됨)

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include "blockmapping.h"
// 필요한 경우 헤더 파일을 추가하시오.

// 해시 테이블의 각 노드를 나타내는 구조체
typedef struct HashNode
{
    int lbn;
    int pbn;
    struct HashNode *next; // 다음 노드를 가리키는 포인터
} HashNode;

// 해시 테이블을 나타내는 구조체
typedef struct
{
    HashNode *table[DATABLKS_PER_DEVICE - 1]; // 포인터 배열로 해시 테이블을 구현
} Hashtable;

void initHashtable(Hashtable *ht)
{
    for (int i = 0; i < DATABLKS_PER_DEVICE - 1; i++)
    {
        ht->table[i] = NULL;
    }
}
// 키-값 쌍을 해시 테이블에 추가하는 함수
void insert(Hashtable *ht, int lbn, int pbn)
{
    unsigned int index = lbn % (DATABLKS_PER_DEVICE - 1);
    HashNode *new_node = (HashNode *)malloc(sizeof(HashNode));
    if (!new_node)
    {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }
    new_node->lbn = lbn;
    new_node->pbn = pbn;
    new_node->next = ht->table[index];
    ht->table[index] = new_node;
}

// 주어진 lbn에 대응하는 pbn을 찾아주는 함수
int getPbn(Hashtable *ht, int lbn)
{
    unsigned int index = lbn % (DATABLKS_PER_DEVICE - 1); // 해시 테이블에서의 인덱스 계산
    HashNode *current = ht->table[index];
    while (current)
    {
        if (current->lbn == lbn)
        {
            return current->pbn; // 해당 lbn에 대응하는 pbn 반환
        }
        current = current->next;
    }
    return -1; // 해당 lbn에 대응하는 pbn이 없을 경우 -1 반환
}

// 주어진 lbn에 대응하는 pbn을 변경하는 함수
void updatePbn(Hashtable *ht, int lbn, int new_pbn)
{
    unsigned int index = lbn % (DATABLKS_PER_DEVICE - 1); // 해시 테이블에서의 인덱스 계산
    HashNode *current = ht->table[index];
    while (current)
    {
        if (current->lbn == lbn)
        {
            // 해당 lbn에 대응하는 pbn을 변경
            current->pbn = new_pbn;
            return;
        }
        current = current->next;
    }
}

// Free block 링크드 리스트

typedef struct Block
{
    int address;        // 블록의 시작 주소
    struct Block *next; // 다음 블록을 가리키는 포인터
} Block;

typedef struct
{
    Block *head; // 리스트의 첫 번째 블록을 가리키는 포인터
} FreeList;

// 링크드 리스트 초기화
void initFreeList(FreeList *flist)
{
    flist->head = NULL;
}
// 블록을 링크드 리스트에 추가
void addBlock(FreeList *flist, int address)
{
    Block *new_block = (Block *)malloc(sizeof(Block));
    if (!new_block)
    {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    new_block->address = address;
    new_block->next = flist->head;
    flist->head = new_block;
}

// 리스트의 헤드를 반환하는 함수
Block *getHead(FreeList *list)
{
    return list->head;
}
// 리스트의 헤드를 삭제하는 함수
void deleteHead(FreeList *list)
{
    if (list->head == NULL)
    {
        // 리스트가 비어있는 경우 아무 작업도 하지 않음
        return;
    }
    Block *temp = list->head;
    list->head = list->head->next;
    free(temp);
}

// 링크드 리스트의 모든 블록을 제거하고 메모리 해제
void clearFreeList(FreeList *flist)
{
    Block *current = flist->head;
    while (current)
    {
        Block *temp = current;
        current = current->next;
        free(temp);
    }
    flist->head = NULL;
}

//
// flash memory를 처음 사용할 때 필요한 초기화 작업, 예를 들면 address mapping table에 대한
// 초기화 등의 작업을 수행한다. 따라서, 첫 번째 ftl_write() 또는 ftl_read()가 호출되기 전에
// file system에 의해 반드시 먼저 호출이 되어야 한다.
//

Hashtable ht;
FreeList flist;

void ftl_open()
{
    //
    // address mapping table 초기화 또는 복구

    initHashtable(&ht);
    for (int i = 0; i < DATABLKS_PER_DEVICE - 1; i++)
    {
        insert(&ht, i, -1);
    }
    // free block's pbn 초기화

    initFreeList(&flist);
    addBlock(&flist, 0);

    // address mapping table에서 lbn 수는 DATABLKS_PER_DEVICE 동일

    return;
}

//
// 이 함수를 호출하는 쪽(file system)에서 이미 sectorbuf가 가리키는 곳에 512B의 메모리가 할당되어 있어야 함
// (즉, 이 함수에서 메모리를 할당 받으면 안됨)
//
void ftl_read(int lsn, char *sectorbuf)
{
    int lbn = 0;
    int offset = 0;
    int pbn = -1;
    int ppn = 0;
    char pagebuf[PAGE_SIZE];

    // lsn -> lbn
    lbn = lsn / PAGES_PER_BLOCK;
    offset = lsn % PAGES_PER_BLOCK;

    // AMT에서 lbn=2에 해당하는 pbn가져오기
    if ((pbn = getPbn(&ht, lbn)) == -1)
    {
        fprintf(stderr, "ERROR: pbn error write first!\n");
        exit(1);
    }

    ppn = 4 * pbn + offset;
    // 페이지 단위로 가져와서
    if (dd_read(ppn, pagebuf) == -1)
    {
        fprintf(stderr, "ERROR: dd_read error\n");
        exit(1);
    }
    // sector만 쏙
    memcpy(sectorbuf, pagebuf, SECTOR_SIZE);

    return;
}

//
// 이 함수를 호출하는 쪽(file system)에서 이미 sectorbuf가 가리키는 곳에 512B의 메모리가 할당되어 있어야 함
// (즉, 이 함수에서 메모리를 할당 받으면 안됨)
//

void ftl_write(int lsn, char *sectorbuf)
{
    int lbn = 0;
    int offset = 0;

    // lsn -> lbn
    lbn = lsn / PAGES_PER_BLOCK;
    offset = lsn % PAGES_PER_BLOCK;

    char pagebuf[PAGE_SIZE];
    char sparebuf[SPARE_SIZE];
    char spare[SPARE_SIZE];
    int pbn;
    int ppn;

    // 1) AMT에  매핑 된게 없다.
    if ((pbn = getPbn(&ht, lbn)) == -1)
    {

        // pbn의  모든 페이지의 spare가 비어있는지 확인하기
        for (int i = 1; i < BLOCKS_PER_DEVICE - 1; i++)
        {
            int isInFreeList = 0;
            // flist에 있는 블록이라면 다음 후보 선택
            Block *current = getHead(&flist);
            while (current)
            {
                if (current->address == i)
                {
                    isInFreeList = 1;
                    break;
                }
                current = current->next;
            }
            if (isInFreeList)
            {
                continue;
            }
            int isEmpty = 1;
            for (int j = 0; j < PAGES_PER_BLOCK; j++)
            {

                memset(pagebuf, 0xFF, PAGE_SIZE);

                ppn = i * PAGES_PER_BLOCK + j;

                //@todo spare만큼 가져오기
                if (dd_read(ppn, pagebuf) == -1)
                {
                    fprintf(stderr, "ERROR: dd_read error\n");
                    exit(1);
                }

                // 1) spare에 데이터가 없다면 i를 블록으로 지정
                if (pagebuf[SECTOR_SIZE] != (char)0xFF)
                {
                    isEmpty = 0;
                }
            }
            if (isEmpty == 1)
            {
                pbn = i;
                break;
            }
        }

        // 2-1) pbn이 -1이 아니라면 -> 해당 pbn에 데이터 입력 (덮어쓰기가 아니라 제대로 할당 된것)
        if (pbn != -1)
        {

            memset(pagebuf, 0, PAGE_SIZE);
            memcpy(pagebuf, sectorbuf, SECTOR_SIZE);
            memset(sparebuf, 0, SPARE_SIZE);
            memcpy(sparebuf, &lsn, sizeof(int));

            memcpy(pagebuf + SECTOR_SIZE, sparebuf, SPARE_SIZE);

            ppn = pbn * PAGES_PER_BLOCK + offset;

            if (dd_write(ppn, pagebuf) == -1)
            {
                fprintf(stderr, "ERROR: dd_write error\n");
                exit(1);
            }
            // freeblock의 address를 AMT에 매핑
            updatePbn(&ht, lbn, pbn);
        }
        else
        {
            // 2-2) pbn이 -1이라면 -> 해당 pbn을 못쓰는 것이기에 빈블록에 할당해야

            // free block에 데이터 복사하고 주소 바꿔주기
            Block *freeblock = getHead(&flist);
            int freeblockaddress = freeblock->address;
            // 빈블록 할당했으면 헤드 삭제
            deleteHead(&flist);

            for (int i = 0; i < PAGES_PER_BLOCK; i++)
            {
                if (i == offset)
                {
                    continue;
                }
                // offset제외하고 나머지는 빈 블록으로 옮기기
                int oldppn = PAGES_PER_BLOCK * pbn + i;
                memset(pagebuf, 0, PAGE_SIZE);
                if (dd_read(oldppn, pagebuf) == -1)
                {
                    fprintf(stderr, "ERROR: dd_read error\n");
                    exit(1);
                }
                int freeppn = PAGES_PER_BLOCK * freeblockaddress + i;
                if (dd_write(freeppn, pagebuf) == -1)
                {
                    fprintf(stderr, "ERROR: dd_write error\n");
                    exit(1);
                }
            }
            // 원래 pbn erase
            if (dd_erase(pbn) == -1)
            {
                fprintf(stderr, "ERROR: dd_erase error\n");
                exit(1);
            }

            // freeblock의 address를 AMT에 매핑
            updatePbn(&ht, lbn, freeblockaddress);

            // 원래 pbn을 free block list에 넣기
            addBlock(&flist, pbn);
        }
    }
    else
    {
        // 1-1) AMT에 매핑된게 있다.
        pbn = getPbn(&ht, lbn);

        memset(pagebuf, 0, PAGE_SIZE);

        ppn = pbn * 4 + offset;

        if (dd_read(ppn, pagebuf) == -1)
        {
            fprintf(stderr, "ERROR: dd_read error\n");
            exit(1);
        }

        // spare에 lsn이 없다면
        if ((unsigned char)pagebuf[SECTOR_SIZE] == 0xFF)
        {

            memset(pagebuf, 0, PAGE_SIZE);
            memcpy(pagebuf, sectorbuf, SECTOR_SIZE);
            memset(sparebuf, 0, SPARE_SIZE);
            memcpy(sparebuf, &lsn, SPARE_SIZE);

            memcpy(pagebuf + SECTOR_SIZE, sparebuf, SPARE_SIZE);

            if (dd_write(ppn, pagebuf) == -1)
            {
                fprintf(stderr, "ERROR: dd_write error\n");
                exit(1);
            }

            // freeblock의 address를 AMT에 매핑
            updatePbn(&ht, lbn, pbn);
        }
        else
        { // spare에 lsn이 있다면

            // free block에 데이터 복사하고 주소 바꿔주기
            Block *freeblock = getHead(&flist);

            int freeblockaddress = freeblock->address;
            // 빈블록 할당했으면 헤드 삭제
            deleteHead(&flist);

            for (int i = 0; i < PAGES_PER_BLOCK; i++)
            {
                if (i == offset)
                {
                    continue;
                }
                // offset제외하고 나머지는 빈 블록으로 옮기기
                int oldppn = PAGES_PER_BLOCK * pbn + i;
                memset(pagebuf, 0, PAGE_SIZE);
                if (dd_read(oldppn, pagebuf) == -1)
                {
                    fprintf(stderr, "ERROR: dd_read error\n");
                    exit(1);
                }
                int freeppn = PAGES_PER_BLOCK * freeblockaddress + i;
                if (dd_write(freeppn, pagebuf) == -1)
                {
                    fprintf(stderr, "ERROR: dd_write error\n");
                    exit(1);
                }
            }

            // 원래 ppn도 새로운 주소에
            ppn = pbn * PAGES_PER_BLOCK + offset;
            if (dd_read(ppn, pagebuf) == -1)
            {
                fprintf(stderr, "ERROR: dd_read error\n");
                exit(1);
            }
            int newoffsetppn = PAGES_PER_BLOCK * freeblockaddress + offset;

            if (dd_write(newoffsetppn, pagebuf) == -1)
            {
                fprintf(stderr, "ERROR: dd_write error\n");
                exit(1);
            }
            // 원래 pbn erase
            if (dd_erase(pbn) == -1)
            {
                fprintf(stderr, "ERROR: dd_erase error\n");
                exit(1);
            }

            // freeblock의 address를 AMT에 매핑
            updatePbn(&ht, lbn, freeblockaddress);

            // 원래 Pbn을 free block list에 넣기
            addBlock(&flist, pbn);
        }
    }

    return;
}

void ftl_print()
{
    printf("lbn pbn\n");
    for (int i = 0; i < DATABLKS_PER_DEVICE - 1; i++)
    {
        HashNode *current = ht.table[i];
        while (current)
        {
            printf("%d %d\n", current->lbn, current->pbn);
            current = current->next;
        }
    }
    Block *freeblock = getHead(&flist);
    printf("free block=%d\n", freeblock->address);

    return;
}
