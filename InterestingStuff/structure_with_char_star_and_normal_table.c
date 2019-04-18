#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <string.h>

char *date_time = NULL; //calloc(100, sizeof(char));
struct date{
    char year[5]; //jeśli mają być 4 elementy, to trzeba zrobić 5, bo '\0'!!; ew. 
                  //dynamiczna alokacja, tj. char *year;
    char month[3];
    char day[3];
};

int set_variables(char **argv){
    date_time = argv[1]; //strcpy(date_time, argv[1]);
    if(strlen(date_time) != 10){
        printf("Incorrect data format. Follow this one: 'yyyy-mm-dd'\n");
        return -1;
    }

    struct date *date = calloc(sizeof(date), sizeof(char*)); //!
    //date->year = calloc(sizeof(date->year), sizeof(char)); //dynamiczna alokacja

    strncat(date->year, date_time, 4); //append 4 values from date_time to date
    printf("%s\n", date->year);
    
    strncat(date->month, date_time+5, 2); //append 4 values from date_time to date
    printf("%s ", date->year);
    printf("%s\n", date->month);

    strncat(date->day, date_time+8, 2); //append 4 values from date_time to date
    printf("%s ", date->year);
    printf("%s ", date->month);
    printf("%s\n", date->day);
    
    
    return 0;
}

int main(int argc, char **argv) {
    if(set_variables(argv) < 0)
        return -1;
    
}