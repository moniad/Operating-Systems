// // not workng as I wished
// #include <errno.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <unistd.h>
// #include <sys/types.h>
// #include <sys/wait.h>

// int main(int argc, char* argv[]) {
//     char *buf = malloc(100 * sizeof(char));
//     int fd[2], fd1[2];
//     pipe(fd);
//     pipe(fd1);
//     pid_t pid = fork();
//     if (pid == 0) { // dziecko
//         close(fd[1]);
//         read(fd[0], buf, 100);
//         printf("%s\n", buf);
        
//         // dup2(fd1[1],STDOUT_FILENO);
//         // system(command > temp1)
//         // chciałam, żeby mi wypisywało do pliku, a nie STDOUT
//         execlp("grep", "grep", buf, ".", NULL);
//     } else { // rodzic
//         close(fd[0]);
//         write(fd[1], "*.c", 10); // - przesłanie danych do grep-a

//         dup2(fd1[0],STDIN_FILENO);
//         close(fd1[1]);
//     }
//     return 0;
// }

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

int main(int argc, char* argv[]) {
    char *buf = malloc(100 * sizeof(char));
    int fd[2];
    pipe(fd);
    pid_t pid = fork();
    if (pid == 0) { // dziecko
        close(fd[1]); 
        dup2(fd[0],STDIN_FILENO);  
        // read(fd[0], buf, 100);
        // printf("BUF: %s\n", buf);
        execlp("grep", "grep", "status", NULL);
    } else { // rodzic
        close(fd[0]);
        dup2(fd[1],STDOUT_FILENO);
        char *str1 = malloc(sizeof(char) * 20);
        strcpy(str1, "get_child_status.c\n");
        write(fd[1], str1, strlen(str1)); // - przesłanie danych do grep-a
        strcpy(str1, "status\n");
        write(fd[1], str1, strlen(str1)); // !!! important
        while(1);
    }
    return 0;
}
