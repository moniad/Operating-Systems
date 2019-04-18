// not working properly - it doesn't kill this process, but the command gets processed as such
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char *argv[]) {
    const char *signal = "KILL"; // "USR1"; // "SIGUSR1";
    // printf("Signal no: %d", (int) strtol(signal, NULL, 10));
    char *command = malloc(30 * sizeof(char));
    strcpy(command, "kill -s ");
    strcat(command, signal);
    strcat(command, " ");
    char *pid = malloc(6 * sizeof(char));
    sprintf(pid, "%d\n", 8295);
    printf("pid: %s", pid);
    strcat(command, pid);
    printf("COMMAND: %s\n", command);
    system(command);
    return 0;
}