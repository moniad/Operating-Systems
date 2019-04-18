#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <string.h>

char *date_time = NULL; // = calloc(100, sizeof(char));

int set_variables(char **argv){
    date_time = argv[1]; //strcpy(date_time, argv[1]);
    if(strlen(date_time) != 10){
        printf("Incorrect data format. Follow this one: 'yyyy-mm-dd'\n");
        return -1;
    }

    char *date = calloc(10, sizeof(char)); //!
    strncat(date, date_time, 4); //append 4 values from date_time to date
    printf("%s\n", date);
    
    return 0;
}

int main(int argc, char **argv) {
    if(set_variables(argv) < 0)
        return -1;
    
}

// wydaje mi się, że to jest źle. miałam przez takie cosie błędy. chyba powinno być tak
// jak w nawiasach