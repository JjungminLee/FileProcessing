#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

// a.out <오프셋> <데이터> <파일명>
int main(int argc, char *argv[]) {
    
    off_t offset = atoi(argv[1]); 
    char *data = argv[2]; 
    char *filename = argv[3]; 
    int fd;

    if (argc < 4) {
        fprintf(stderr, "Usage: %s <offset> <data> <filename>\n", argv[0]);
        return 1;
    }
    
    // 큰따옴표 제거
    size_t dataLen = strlen(data);
    if (data[0] == '"' && data[dataLen - 1] == '"') {
        data[dataLen - 1] = '\0'; // 마지막 큰따옴표를 NULL로 대체
        data += 1; // 첫 번째 큰따옴표를 건너뛰기
        dataLen -= 2; // 양 끝의 큰따옴표 제거로 길이 조정
    }
    if ((fd = open(filename, O_RDWR | O_CREAT, 0644))<0) {
        fprintf(stderr,"ERROR: find open error\n");
        exit(1);
    }
    off_t filesize;
    filesize = lseek(fd,0,SEEK_END);


    if(offset>filesize){
        fprintf(stderr,"ERROR: Cannot overwrite!\n");
        exit(1);
    }

    // 오프셋으로 이동
    lseek(fd, offset, SEEK_SET);
    // 데이터 쓰기 
    // 입력 받은 데이터 크기만큼 쓰지 않으면 에러
    if (write(fd, data, dataLen) != dataLen) {
        fprintf(stderr,"ERROR: Error writing to file\n");
        close(fd);
        exit(1);
    }

    close(fd); // 파일 닫기
    exit(0);
}
