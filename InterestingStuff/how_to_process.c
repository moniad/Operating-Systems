if(file_type == DT_DIR){
    pid_t child; // = fork();
    if((child = fork()) == 0){ // child process
        open_and_list_dirs_using_DIR(path_to_file);
        exit(0); // VITALLY IMPORTANT!!!!
    }
}