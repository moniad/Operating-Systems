#include <stdio.h>
#include <stdlib.h>

char *date_time = NULL;
char *option = NULL;
char *path_to_dir = NULL;

void list_dir_using_find(char **argv){
    //size_t size = 512;
    //char *command = calloc(size, sizeof(char));
    
    //-P - Never follow symbolic links.  This is the default behaviour.
    //-newer File was modified more recently than file.
    //! negates the next condition:
    //files newer than day A and not older than day after day A = files from day A
//stuff for listing dir
    /*
    if(argv[2][0] == '<') //earlier modification date MEANS OLDER FILES
        snprintf(command, 512, "find %s -type f ! -newermt '%s' -ls", path_to_dir, date_time);
    else if(argv[2][0] == '>') //older modification date
        snprintf(command, 512, "find %s -type f -newermt '%s' -ls", path_to_dir, date_time);
    else if(argv[2][0] == '='){
        char *next_date_time = calloc(strlen(date_time), sizeof(char));
        strcpy(next_date_time,date_time);
        //add 1 day to date_time
        next_date_time[strlen(next_date_time)-1] ++;
        snprintf(command, 512, "find %s -type f -newermt '%s' ! -newermt '%s' -ls", path_to_dir, date_time, next_date_time);
    }*/
    //printf("%s\n",command);
    //system(command);
    
    //commands
    //find . -type f -newermt 2019-03-01 -ls
    //find . -type f -newermt 2019-03-12 ! -newermt 2019-03-13 -ls - the exact day
    
    // ./main . '>' '2019-03-12'
}

int set_variables(char **argv){
    path_to_dir = argv[1];
    option = argv[2];
    date_time = argv[3];
    if(strlen(date_time) != 10){
        printf("Incorrect data format. Follow this one: 'yyyy-mm-dd'\n");
        return -1;
    }
    return 0;
}

int parse_input(int argc){
    if(argc != 4){
        printf("Give me 3 args: \n1) path_to_dir \n2) '<' | '>' | '=' \n3) mod_time of the files\n");
        return -1;
    }
}

int main(int argc, char **argv){
    printf("Zestaw 2, zad. 2 - ");
    //BEGINOF unnecessary task
    list_dir_using_find(argv);
    //ENDOF unnecessary task
}