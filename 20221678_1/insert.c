#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#define PATHMAX 1024
#define BUFFER_SIZE 4096

//a.out <오프셋> <데이터> <파일명>
int main(int argc, char *argv[]) {
    
    off_t offset = atoi(argv[1]);
    char *inputData = argv[2];
    char *filename = argv[3];
    int fd;
    char *buf = (char *)malloc(sizeof(char) * BUFFER_SIZE);
    if (argc <4) {
        printf("Usage: %s <offset> <data> <filename>\n", argv[0]);
        return 1;
    }
    // 큰따옴표 제거 로직
    char *data;
    int inputDataLen = strlen(inputData);
    if (inputData[0] == '"' && inputData[inputDataLen - 1] == '"') {
        inputData[inputDataLen - 1] = '\0'; // 마지막 큰따옴표 제거
        data = inputData + 1; // 첫 번째 큰따옴표 제거
    } else {
        data = inputData;
    }
    if((fd=open(filename,O_RDWR))<0){
        fprintf(stderr,"ERROR: file open error!\n");
    }

     // 파일 크기 측정
    off_t fileSize = lseek(fd, 0, SEEK_END);
    if (offset >= fileSize) {
        // 오프셋이 파일 크기보다 크거나 같으면, 바로 append
        write(fd, data, strlen(data));
    } else {
      
        long dataLen = strlen(data);
        // 인자로 받아온 offset만큼 이동
        lseek(fd, offset, SEEK_SET);
        // offset부터 끝까지 읽기
        read(fd, buf, fileSize - offset);
        // 삽입할 데이터 위치로 이동
        lseek(fd, offset, SEEK_SET);
        // 데이터 길이 만큼 쓰기
        write(fd, data, dataLen);
        // 밀려난 데이터 쓰기
        write(fd, buf, fileSize - offset);

        free(buf);
        
    }
    close(fd); 
    exit(0);
}
