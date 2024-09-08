#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#define PATHMAX 1024
#define BUFFER_SIZE 4096

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <offset> <nbytes> <filename>\n", argv[0]);
        exit(1);
    }

    char *buf = (char *)malloc(sizeof(char) * BUFFER_SIZE);
    if (!buf) {
        perror("Malloc failed");
        exit(1);
    }
    
    off_t offset = atoi(argv[1]);
    int nbytes = atoi(argv[2]); 
    char filepath[PATHMAX];
    strcpy(filepath, argv[3]);
    int tmpOffset = offset;
    ssize_t count;

    int fd = open(filepath, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "ERROR: file open error %s\n", filepath);
        free(buf);
        exit(1);
    }
    if (nbytes > 0) {
        lseek(fd, offset, SEEK_SET);
        count = read(fd, buf, nbytes);
    } else if (nbytes < 0) {
        nbytes*=-1;
        if(nbytes>offset){
            lseek(fd, 0, SEEK_SET);
            count = read(fd, buf, tmpOffset);
        }else{
            lseek(fd, offset-nbytes, SEEK_SET);
            count = read(fd, buf, nbytes);
        }
        
    }
    if (count >= 0) {
        buf[count] = '\0'; // 문자열 끝 처리
        printf("%s\n", buf);
    } else {
        perror("Read error");
    }

    close(fd);
    free(buf);
    return 0;
}
