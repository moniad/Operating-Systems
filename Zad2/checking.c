// It works! <- poprawić, nie jest do końca dobrze
#define _GNU_SOURCE //needed for FTW_PHYS and strptime() to work
#include <signal.h> // sigaction()
#include <stdio.h>
#include <string.h>
#include <stdlib.h> //calloc(), system(), exit()
#include <sys/stat.h> //stat()
#include <unistd.h> //getcwd(), sleep()
#include <time.h> //time_t, strptime(), ctime()
#include <fcntl.h> //open()
#include <sys/wait.h>
#include <sys/types.h>
#include <errno.h>
#include <libgen.h> // basename()
#include <linux/limits.h> // PATH_MAX
#include <dirent.h> // DIR

const int REC_SIZE = 1024;
const int MAX_FILE_NO = 1024;
// @down - kwestia fork-a...
/// z tym robię!!!!, to nie działa, bo problem z modyfikowaniem tablicy globalnej wewnątrz funkcji -.- wraca do punktu wyjścia... 
int numOfCopies[1024];
// int runOrNot[MAX_FILE_NO];

int runOrNot = 1;


struct monitoredFile{
    char *path_to_file; // rel path
    int time; // monitor file every "time" seconds
    int pid;
    int isMonitored;
    int noOfCopies;
};

struct tableOfMonitoredFiles{
    struct monitoredFile **monitored_files;
    int size;

};

int find_proc_numb(struct tableOfMonitoredFiles *table, pid_t pid);

int get_no_of_lines(char *path_to_file){
    FILE *file = fopen(path_to_file, "r");
    if(!file){
        printf("Could not open file");
        return -1;
    }
    char *line = calloc(REC_SIZE, sizeof(char));
    int no_of_lines = 0;

    while(fgets(line, REC_SIZE, file)) // read from file to line
        no_of_lines++;

    fclose(file);
    return no_of_lines;
}

size_t get_file_size(char *file_name){
    FILE *file = fopen(file_name, "r");
    if(!file){
        printf("Couldn't open file!\n");
        return -1;
    }
    //SEEK_END moves file pointer position to the end of file
    fseek(file, 0, SEEK_END);
    size_t size = (size_t) ftell(file); //returns the position of file ptr
    fclose(file);

    return size;
}

char *return_file_name(char *path_to_file){
    // basename() - to extract file_name from path
    char *file_to_monit = calloc(REC_SIZE, sizeof(char));
    file_to_monit = basename(path_to_file);
    return file_to_monit;
}

time_t get_m_time(struct monitoredFile *monitored_file){
    struct stat file_info;
    //setting file_info attrbs
    if(stat(monitored_file->path_to_file, &file_info) < 0){
        printf("ERRNO CHANGED: %d ", errno);
        if(errno == EBADF){
            printf("Bad file descriptor.");
            return -1;
        }
        else if(errno == ENOENT){
            printf("File %s does not exist.\n", return_file_name(monitored_file->path_to_file));
            return -1;
        }
    }
    // printf("\n\nST M TIME: %ldd\n\n", file_info.st_mtime);
    time_t m_time = file_info.st_mtime;
    return m_time;
}

char *convert_time(time_t time_to_be_conv){
    char *time = malloc(sizeof(char) * 26);
    // localtime converts time passed from 1970 (in seconds) to struct tm
    // which is needed by strftime to convert it to string using format
    strftime(time, 26, "_%F_%H-%M-%S", localtime(&time_to_be_conv));
    return time;
}

struct monitoredFile **init_monitored_files_data(char *path_to_file){
    int no_of_lines = get_no_of_lines(path_to_file);
    struct monitoredFile **monitored_files = malloc(no_of_lines * sizeof(struct monitoredFile *));
    for(int i=0; i<no_of_lines; i++)
        monitored_files[i] = malloc(sizeof(struct monitoredFile) + sizeof(char) * PATH_MAX);

    FILE *file = fopen(path_to_file, "r");
    if(!file){
        printf("Could not open file");
        return NULL;
    }

    char *line = calloc(REC_SIZE, sizeof(char));
    int i = 0;
    while(fgets(line, REC_SIZE, file)){ // read from file to line
        char *path_to_file = realpath(strtok(line, " "), NULL);
        monitored_files[i]->path_to_file = path_to_file;
        if(!monitored_files[i]->path_to_file){
            printf("Provide valid file name!");
            return NULL;
        }
        monitored_files[i]->time = (int) strtol(strtok(NULL, " "), NULL, 10); // suppose time > 0
        monitored_files[i]->isMonitored = 1;
        monitored_files[i]->noOfCopies = 0;
        i++;
    }
    fclose(file);
    return monitored_files;
}

char *get_file_content(char *path_to_file){
    size_t file_size = get_file_size(path_to_file);
    char *file_content = calloc(file_size+1, sizeof(char));
    // printf("\nFILE SIZE: %lld", (long long) file_size);
    // reading file_content
    FILE *file = fopen(path_to_file, "r");

    if(fread(file_content, sizeof(char), file_size, file) != file_size){
        printf("Sth went wrong during reading data from file");
        return NULL;
    }

    fclose(file);
    return file_content;
}

void handleMySIGSTOP(int signum){
    printf("\nOdebrano sygnał SIGUSR1 - STOP, PID: %d\n", getpid());
    runOrNot = 0;
}

void handleMySIGCONT(int signum){
    printf("\nOdebrano sygnał SIGUSR2 - CONTINUE, PID: %d\n", getpid());
    runOrNot = 1;
}

int find_proc_numb(struct tableOfMonitoredFiles *table, pid_t pid){
    int index = 0;
    int found = 0;
    while(index<table->size){
        if(table->monitored_files[index]->pid == pid){
            table->monitored_files[index]->isMonitored = 0;
            break;
        }
        index++;
    }
    if(index == table->size)
        printf("Not found!\n");
    printf("returning with index :)\n");
    return index;
}

void print_numOfCopies(){
    printf("Printing numOfCOpies!!!:\n");
    for(int i=0; i<10; i++)
        printf("%d ", numOfCopies[i]);
    printf("\n");
}
int monitor_memory_ver(struct tableOfMonitoredFiles *table, struct monitoredFile *monitored_file){
    printf("Started monitoring!!\n");
    signal(SIGUSR1, handleMySIGSTOP);
    signal(SIGUSR2, handleMySIGCONT);
    time_t last_m_time = get_m_time(monitored_file);
    // printf("PID: %d LAST M TIME: %ul\n\n", getpid(), (unsigned long) last_m_time);

//* get file_content
    char *file_content = get_file_content(monitored_file->path_to_file);
    if(!file_content) return -1;
    time_t cur_m_time;
    while(1){
        // printf("IS OR NOT: %d ", monitored_file->isMonitored);
        // if(monitored_file->isMonitored){ // enters this if even though it mustn't!!!
        if(runOrNot){
            cur_m_time = get_m_time(monitored_file);

            printf("PID %d, CUR MODIFICATION TIME: %lld\n", getpid(), (long long) cur_m_time);
            printf("PID %d, LAST M TIME %d VS CUR M TIME %d\n", getpid(), last_m_time, cur_m_time);
            if(last_m_time < cur_m_time){
                printf("SAVING MODIFICATIONS...\n");
                char *file_name = malloc(sizeof(char) * PATH_MAX);
                strcpy(file_name, return_file_name(monitored_file->path_to_file));
                strcat(file_name, convert_time(cur_m_time));

                // printf("PID %d, FILE NAME: %s\n", getpid(), file_name);

                DIR *dir = opendir("archiwum");
                if(!dir) mkdir("archiwum", 0774); // mode rwxrwxr--

                chdir("archiwum");
                // writing to file
                FILE *file = fopen(file_name, "w");
                if(!file){
                    printf("Could not read file\n");
                    return -1;
                }
                size_t file_size = get_file_size(monitored_file->path_to_file);
                
                if(fwrite(file_content, sizeof(char), (size_t) (file_size+1), file) != file_size+1){
                    printf("Sth went wrong during writing data\n");
                    return -1;
                }
                fclose(file);
                chdir("..");
                printf("SAVED.\n");
                last_m_time = cur_m_time;
                file_content = get_file_content(monitored_file->path_to_file);
                monitored_file->noOfCopies++;
                int index = 0;
                printf("index: %d ", index);
                printf("getpid(): %d   ", getpid());
                index = find_proc_numb(table, getpid());
                // printf("index: %d   ", index);
                numOfCopies[index] = monitored_file->noOfCopies;
                printf("COPIES: %d ", numOfCopies[index]);
                printf("NO OF COPIES OF THE FILE NAMED %s DONE: %d\n", monitored_file->path_to_file, monitored_file->noOfCopies);
                print_numOfCopies();
                closedir(dir);
                free(file_name);
            }

            sleep((unsigned int) monitored_file->time);
        }
        printf("SLEEPY\n");
        sleep(1);
    }
    free(file_content);
    exit(monitored_file->noOfCopies);
    // return 0; // monitored_file->noOfCopies; // it will never happen
} 

void list_all_processes(struct tableOfMonitoredFiles *table){
    printf("Printing all children PIDs:\n");
    for(int i=0; i<table->size; i++)
        printf("PID: %d, FILE: %s, isMonitored %d\n", table->monitored_files[i]->pid, 
        table->monitored_files[i]->path_to_file, table->monitored_files[i]->isMonitored);
    // printf("\nEnded!\n");
}

void end_all_and_report(struct tableOfMonitoredFiles *table){
    // int status; // = malloc(sizeof(int));
    pid_t child_pid;

    print_numOfCopies();
    
    for(int i=0; i<table->size; i++){
        child_pid = table->monitored_files[i]->pid;
        printf("KILLED PROCES: %d, FILE: %s, ", child_pid, table->monitored_files[i]->path_to_file);
        printf("NO. OF COPIES: %d\n", numOfCopies[i]);
        // printf("Don't know how many copies (...)\n"); // have been created because I can't return anything from killed process and C language does not support passing by reference :/\n");
// ilość kopii w noOfCopies się nie zapisała ._.
        kill(child_pid, SIGINT);
    }
}

void monitor(struct tableOfMonitoredFiles* table){
    char command[20];

    while(1){
        scanf("%s", command);
        printf("COMMAND: %s\n", command);
        if(strcmp(command, "LIST") == 0)
            list_all_processes(table);
        else if(strcmp(command, "STOP") == 0){
            scanf("%s", command);
            if(strcmp(command, "ALL") == 0){
                printf("ALL PROCESSES WILL BE STOPPED.\n");
                pid_t pid;
                for(int i=0; i<table->size; i++){
                    pid = table->monitored_files[i]->pid;
                    table->monitored_files[i]->isMonitored = 0;
                    kill(pid, SIGUSR1);
                }
            }
            else { // we have PID
                pid_t pid = (int) strtol(command, NULL, 10);
                printf("PID OF STOPPED PROCESS: %d\n", pid);
                // stop monitoring pid:
                int index = find_proc_numb(table, pid);
                printf("index: %d\n", index);
                if(index != -1){
                    table->monitored_files[index]->isMonitored = 0;
                    printf("PID: %d\n", table->monitored_files[index]->pid);
                    kill(table->monitored_files[index]->pid, SIGUSR1);
                }
            }
        }
        else if(strcmp(command, "START") == 0){
            scanf("%s", command);
            if(strcmp(command, "ALL") == 0){
                 pid_t pid;
                printf("ALL PROCESSES WILL BE CONTINUED.\n");
                for(int i=0; i<table->size; i++){
                    pid = table->monitored_files[i]->pid;
                    table->monitored_files[i]->isMonitored = 1;
                    kill(pid, SIGUSR2);
                }
            }
            else{
                pid_t pid = (int) strtol(command, NULL, 10);
                printf("PID OF PROCESS THAT WILL BE CONTINUED: %d\n", pid);
                // start monitoring pid:
                int index = find_proc_numb(table, pid);
                if(index != table->size){
                    table->monitored_files[index]->isMonitored = 1;
                    kill(table->monitored_files[index]->pid, SIGUSR2);
                }
            }
        }
        else if(strcmp(command, "END") == 0){
            end_all_and_report(table);
// this line might not be there
            break;
        }
        else printf("Command not recognized. Try again!\n");

        sleep(1);
    }
}

int check_input(int argc, char **argv){
    if(argc != 2){
        printf("Give me 1 arg: \n1) file_name\n");
        return -1;
    }
    return 0;
}


int main(int argc, char **argv) {
    
    if(check_input(argc, argv) < 0)
        return -1;
    
    //parse input
    char *file_name = calloc(REC_SIZE, sizeof(char));
    strcpy(file_name, argv[1]);
    struct tableOfMonitoredFiles *table = malloc(sizeof(struct tableOfMonitoredFiles));
    table->monitored_files = init_monitored_files_data(file_name);
    table->size = get_no_of_lines(file_name);
// printf("tu dodałam tą tablicę!!!!!!!!!!!!!!!");
    // numOfCopies = calloc(MAX_FILE_NO, sizeof(char));
   
    print_numOfCopies();
    printf("I'M CREATING PROCESSES\n");
    for(int i=0; i<table->size; i++){
        pid_t child;
        if((child=fork()) == 0){
            // printf("i: %d, path_to_file: %s, time: %d\n", i, table->monitored_files[i]->path_to_file, table->monitored_files[i]->time);
            printf("i: %d", i);
            monitor_memory_ver(table, table->monitored_files[i]); // < 0){
            //     printf("Something has gone wrong!\n");
            //     return -1;
            // }
        }
        else{ // IMPORTANT! Parent gets child's pid!
            // printf("GET CHILD PID: %d\n", child);
            table->monitored_files[i]->pid = child;
            table->monitored_files[i]->isMonitored = 1;
        }
    }
    print_numOfCopies();
    monitor(table);

    free(file_name);
    for(int i=0; i<table->size; i++)
        free(table->monitored_files[i]);
    free(table);
    return 0;
}