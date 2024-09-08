#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#define PATHMAX 4096
#define BUFFER_SIZE 1024

//a.out <원본파일명> <복사본파일명>
int main(int argc, char *argv[]){

    char *buf =(char *)malloc(sizeof(char) * BUFFER_SIZE);
    char originPath[PATHMAX]="";
    char copyPath[PATHMAX]="";
    int fd1,fd2,length;
    strcpy(originPath,argv[1]);
    strcpy(copyPath,argv[2]);
    
    if(argc<3){
        fprintf(stderr,"Usage: %s <filename> <filename>\n", argv[0]);
        exit(1);
    }
  
    if((fd1=open(originPath,O_RDONLY))<0){
        fprintf(stderr,"ERROR: file open error %s\n",originPath);
        exit(1);
    }
    if((fd2 = open(copyPath, O_WRONLY | O_CREAT | O_TRUNC, 0644))<0){
        fprintf(stderr,"ERROR: file open error %s\n",copyPath);
        exit(1);
    }

    off_t filesize;
    filesize = lseek(fd1,0,SEEK_END);

    lseek(fd1,0,SEEK_SET);
    if(filesize<10){
        while((length = read(fd1,buf,BUFFER_SIZE))>0){
            write(fd2,buf,filesize);
        }

    }else{
        while((length = read(fd1,buf,BUFFER_SIZE))>0){
            write(fd2,buf,10);
        }

    }
    close(fd1);
    close(fd2);
    exit(0);

}