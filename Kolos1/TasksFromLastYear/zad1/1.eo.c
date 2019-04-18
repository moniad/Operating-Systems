#include <errno.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

int main(int argc, char* argv[]) {
    DIR * katalog;
    if (argc!=2)
    {
        printf ("Wywołanie %s ścieżka",argv[0]);
        return 1;
    }
    struct dirent * pozycja;
    katalog = opendir(argv[1]);
    if(!katalog){
        return 1;
    }
    
    char * name = malloc(100 * sizeof(char));
    while((pozycja = readdir(katalog)) != NULL){
        name = pozycja->d_name;
        if((strcmp(name, ".") != 0)  && (strcmp(name, "..") != 0)){
            printf("%s ", name);
        }
        // errno = 3; <- just testing
        if(errno){
            perror(""); // same as below
            fprintf(stderr, "%s ", strerror(errno));
            return 2;
        }
    }
    closedir(katalog);
    free(name);
    return 0;
}
/*Otwórz katalog, w przypadku błędu otwarcia zwróć błąd funkcji otwierającej i zwróć 1. 
Wyświetl zawartość katalogu katalog, pomijając "." i ".."
Jeśli podczas tej operacji wartość errno zostanie ustawiona, zwróć błąd funkcji czytającej oraz wartość 2. */