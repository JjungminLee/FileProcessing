#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

//a.out <오프셋> <바이트 수> <파일명>

int main(int argc, char *argv[]) {
    

    off_t offset = atoi(argv[1]);
    int bytesToDelete = atoi(argv[2]);
    char *filename = argv[3];
    int fd;

    if (argc != 4) {
        printf("Usage: %s <offset> <bytes> <filename>\n", argv[0]);
        exit(1);
    }

    if ((fd = open(filename, O_RDWR))<0) {
        fprintf(stderr,"ERROR: fail to open file");
        exit(1);
    }

    // 파일 크기 측정
    off_t fileSize = lseek(fd, 0, SEEK_END);
    if (bytesToDelete > 0) {
        // 오른쪽 데이터 삭제
        if (offset + bytesToDelete > fileSize) {
            bytesToDelete = fileSize - offset; 
        }
    } else if (bytesToDelete < 0) {
        // 왼쪽 데이터 삭제
        bytesToDelete = -bytesToDelete;
        if (offset - bytesToDelete < 0) {
            bytesToDelete = offset; 
        }
        offset -= bytesToDelete;
    } else {
        // 바이트 수가 0이면 삭제할 필요 없음
        close(fd);
        exit(0);
    }

    // 삭제할 부분 뒤의 데이터를 읽어서 버퍼에 저장
    char *buffer = (char *)malloc(fileSize - offset - bytesToDelete);
    if (buffer == NULL) {
        fprintf(stderr,"Error: Failed to allocate memory\n");
        close(fd);
        return 1;
    }

    lseek(fd, offset + bytesToDelete, SEEK_SET);
    read(fd, buffer, fileSize - offset - bytesToDelete);

    // 삭제할 위치로 이동 후 버퍼의 데이터를 써넣음
    lseek(fd, offset, SEEK_SET);
    write(fd, buffer, fileSize - offset - bytesToDelete);

    // 파일 크기 조정
    ftruncate(fd, fileSize - bytesToDelete);

    free(buffer);
    close(fd);

    return 0;
}
