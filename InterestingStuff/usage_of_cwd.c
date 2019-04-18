 /* usage of cwd
    char pwd[512];
    if(!getcwd(pwd, sizeof(pwd))){ //in cwd there'll be the result of pwd in linux
        perror("getcwd() error");
        return -1;
    }
    */