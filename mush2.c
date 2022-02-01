#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include "mush.h"
#include <sys/types.h>
#include <pwd.h>
#include <signal.h>
#include <fcntl.h>

#define READ 0
#define WRITE 1

void handler(int num){
    /*handles the SIGINT signal*/
    if (isatty(STDIN_FILENO) && isatty(STDOUT_FILENO)){
        /*if interacting with terminal print \n*/
        printf("\n");
    }
    return;
}

void mycd(char* dir){
    /*move to the directory given by dir*/
    if (dir){
        if (chdir(dir) < 0){
            printf("%s: No such file or directory\n", dir);
        }
    }
    else{
        /*if dir is null move to root*/
        struct passwd *pw = getpwuid(getuid());
        if (chdir(pw->pw_dir) < 0){
            perror("error changing to home dir");
            exit(-1);
        }
    }
    return;
}

int main(int argc, char* argv[]){
    char* buf;
    struct sigaction sa;
    pipeline p;
    char prompt[] = "8-P ";
    FILE* infile = stdin;
    int fileflag = 0;

    /*setup handler*/
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    
    if (argc != 2 && argc != 1){
        printf("usage: //home/pn-cs357/demos/mush2 [ -v ] [ infile ]\n");
        exit(-1);
    }
    /*if has input*/
    if (argc == 2){
        if ((infile = fopen(argv[1], "r")) < 0){
            perror("bad file\n");
            exit(-1);
        }
        fileflag = 1;
    }

    if (!fileflag){
        printf("%s", prompt);
    }

    /*while stdin is not at end of file*/
    while (((buf = readLongString(infile)) != NULL) || !feof(infile)){
        if (buf && ((int)buf[0] != 0)){
            /*parse cmd line and eval*/
            if ((p = crack_pipeline(buf))){
                eval_pipeline(stdout, p);
            }
        }
        if (!fileflag){
            printf("%s", prompt);
        }
    }
    if (!fileflag){
        printf("\n");
    }

    yylex_destroy();

    free_pipeline(p);
    free(buf);

    return 0;

}



int eval_pipeline(FILE* file, pipeline p){
    pid_t par_id;
    int filep;
    int len = p->length;
    int pipenum = p->length - 1;
    pid_t id[len];
    int status;
    clstage stage = p->stage;
    int fd[len - 1][2];
    int i;
    int j;


    /*handle cd*/
    if (is_cd(stage[0].argv[0])){
        if (stage[0].argc <= 2){
            mycd(stage[0].argv[1]);
        }
        else{
            printf("usage: cd [ destdir ]\n");
        }
    }
    else{
        /*create pipes*/
        for (i = 0; i < pipenum; i++){
            if (pipe(fd[i]) < 0){
                perror("could not create pipe");
                exit(-1);
            }
        }
        for (i = 0; i < len; i++){
            
            id[i] = fork();
            
            if(id[i] == 0){
                /*if cmd has an in*/
                if (stage[i].inname){

                    if ((filep = open(stage[i].inname, O_RDONLY)) < 0){
                        printf("%s: No such file or directory in read\n", stage[i].argv[0]);
                        return -1;
                    }
                    dup2(filep, STDIN_FILENO);
                }
                /*if cmd has an out*/
                if (stage[i].outname){
                    if ((filep = open(stage[i].outname, O_RDWR | 
                        O_CREAT |O_TRUNC , S_IWUSR |S_IRUSR | S_IRGRP|
                        S_IWGRP |S_IROTH |S_IWOTH)) < 0){
                        printf("error creating file\n");
                        return -1;
                    }
                    dup2(filep, STDOUT_FILENO);
                }
                
                if (i == 0 && pipenum){
                    /*first one*/
                    dup2(fd[i][WRITE], STDOUT_FILENO);
                }
                else if(i == len - 1 && pipenum){
                    /*last one*/
                    dup2(fd[i -1][READ], STDIN_FILENO);
                }
                else if (pipenum){
                    /*in middle*/
                    dup2(fd[i][WRITE], STDOUT_FILENO);
                    dup2(fd[i-1][READ], STDIN_FILENO);
                }
                for(j = 0; j <pipenum; j++){
                    close(fd[j][READ]);
                    close(fd[j][WRITE]);
                }
                /*execute args*/
                if ((execvp(stage[i].argv[0], stage[i].argv)) < 0){
                    perror("No such file or directory in exec\n");
                    exit(-1);
                }
            }
        }
        for(j = 0; j < pipenum; j++){
            close(fd[j][READ]);
            close(fd[j][WRITE]);
        }
        for (i = 0; i < len; i++){
            waitpid(id[i], &status, 0);
        }
    }
}


int is_cd(char* str){
    /*used for determining if stage is cd*/
    char cd[] = "cd";
    if (!strcasecmp(str, cd)){
        return 1;
    }
    return 0;
}


