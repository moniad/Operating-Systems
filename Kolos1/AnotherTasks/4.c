// read last 8 bytes of the file
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(int argc, char *argv[]) {
    const char *file_name = "test";
    int fd = open(realpath(file_name, NULL), O_RDONLY);
    lseek(fd, -8, SEEK_END);
    char *buf = malloc(9 * sizeof(char));
    if(read(fd, buf, 8) != 8) {
        perror("Reading error");
    }
    printf("BUF: %s\n", buf);
    return 0;
}