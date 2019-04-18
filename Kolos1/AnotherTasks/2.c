// some important stuff!!!
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wait.h>
#include <string.h>


int main(int argc, char *argv[]) {
    int toChildFD[2];
    int toParentFD[2];

    pipe(toChildFD);
    pipe(toParentFD);

    int val1 = 8, val2, val3 = 0;

    pid_t child;

    //odczytaj z potoku nienazwanego wartosc przekazana przez proces macierzysty i zapisz w zmiennej val2
    if((child = fork()) == 0){
        close(toChildFD[1]);
        char *str_val2 = malloc(5*sizeof(char));
        read(toChildFD[0], str_val2, 5);
        val2 = (int) strtol(str_val2, NULL, 10);
        printf("read in child VAL2: %d\n", val2);
        // -- second part --
        close(toParentFD[0]);
        write(toParentFD[1], str_val2, strlen(str_val2)); // this is IMPORTANT!!!
    }
    //wyślij val1 potokiem nienazwanym do procesu potomnego
    else {
        close(toChildFD[0]);
        char *str_val1 = malloc(5*sizeof(char));
        sprintf(str_val1, "%d", val1);
        printf("VAL1 - str: %s\n", str_val1);
        write(toChildFD[1], str_val1, strlen(str_val1));
        // -- second part --
        close(toParentFD[1]);
        read(toParentFD[0], str_val1, strlen(str_val1));
        val3 = (int) strtol(str_val1, NULL, 10);
        printf("VAL3: %d\n", val3);
    }
    //odczytaj z potoku nienazwanego wartosc przekazana przez proces potomny i zapisz w zmiennej val3 
    
    return 0;
}
