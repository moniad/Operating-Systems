#include <stdio.h> // printf
#include <stdlib.h> // strtol(), calloc()
#include <unistd.h> // fork()
#include <sys/types.h> // pid_t

int main(int argc, char **argv){

    pid_t child;
    int fd[2]; // file descriptor
    if(pipe(fd) < 0){
        printf("Could not create a pipe\n");
        return -1;
    }
    char *buf = calloc(10, sizeof(char));
    child = fork();
    if(child == 0){
        close(fd[1]); // close fd for writing
        read(fd[0], buf, 4);
        printf("BUFFOR: %s", buf);
    }
    else{
        close(fd[0]); // close for reading
        write(fd[1], "18989890", 4); // write 4 chars, because sizeof(char) = 1B
    }