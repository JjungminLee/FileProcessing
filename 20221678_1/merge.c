#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#define PATHMAX 4096
#define BUFFER_SIZE 1024

//a.out <파일명1> <파일명2> <파일명3>
int main(int argc, char *argv[]){

    char *buf =(char *)malloc(sizeof(char) * BUFFER_SIZE);
    int fd1,fd2,fd3,length;

    if(argc<4){
        fprintf(stderr,"Usage: %s <filename> <filename> <filename>\n", argv[0]);
        exit(1);
    }
  
    if((fd1=open(argv[1],O_RDONLY))<0){
        fprintf(stderr,"ERROR: file open error %s\n",argv[1]);
        exit(1);
    }
    if((fd2=open(argv[2],O_RDONLY))<0){
        fprintf(stderr,"ERROR: file open error %s\n",argv[2]);
        exit(1);
    }
    if((fd3 = open(argv[3], O_WRONLY | O_CREAT | O_TRUNC, 0644))<0){
        fprintf(stderr,"ERROR: file open error %s\n",argv[3]);
        exit(1);
    }

    while((length = read(fd1,buf,BUFFER_SIZE))>0){
        write(fd3,buf,length);
    }
    while((length = read(fd2,buf,BUFFER_SIZE))>0){
        write(fd3,buf,length);
    }
    
    close(fd1);
    close(fd2);
    exit(0);

}