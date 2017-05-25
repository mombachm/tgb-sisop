/*
 ============================================================================
 Name        : main.c
 Author      : Felipe Flores and Matheus Mombach
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <dirent.h>
#include <stdbool.h>

#define O_RDONLY         00
#define O_WRONLY         01
#define O_RDWR           02

const char* DIRETORIO_SCAN = "./scan_files";

int main(void)
{
        int fd[2], nbytes;
        pid_t childpid;
        char string[] = "Hello, world!\n";
        char readbuffer[80];

        pipe(fd);

        if((childpid = fork()) == -1)
        {
                perror("fork");
                exit(1);
        }

        if(childpid == 0)
        {
                /* Child process closes up input side of pipe */
                close(fd[0]);

                /* Send "string" through the output side of pipe */
                write(fd[1], string, (strlen(string)+1));
                exit(0);
        }
        else
        {
                /* Parent process closes up output side of pipe */
                close(fd[1]);

                /* Read in a string from the pipe */
                nbytes = read(fd[0], readbuffer, sizeof(readbuffer));
                printf("Received string: %s", readbuffer);
        }


        listFiles();

        return(0);
}


void listFiles() {
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir (DIRETORIO_SCAN)) != NULL) {
      //varre e exibe todos os arquivos dentro do diretorio informado
      while ((ent = readdir (dir)) != NULL) {

        char filepath[1024];
        if(strcmp(ent->d_name,".")!=0 && strcmp(ent->d_name,"..")!=0) {
            sprintf(filepath, "%s/%s", DIRETORIO_SCAN, ent->d_name);
            printf ("\n%s", /*ent->d_name*/ filepath);
            if(checkFileZip(filepath) == true) {
                printf("\nO arquivo está zipado.");
                //
            }
        }
      }
      closedir (dir);
    } else {
      //erro ao abrir o diretorio
      perror ("");
      return EXIT_FAILURE;
    }
}


bool checkFileZip(char* filepath) {
    int readFd;
    unsigned char readBuffer[2];

    //Cria o descritor de arquivo de leitura
    readFd = open(filepath, O_RDONLY, 0644);
    if(readFd == -1){
        perror("open error");
        return false;
    }

    ssize_t res = read (readFd, readBuffer, 2);

    printf("\n%X", readBuffer[0]);
    printf(" %X", readBuffer[1]);

    if (readBuffer[0]==0x1F && readBuffer[1]==0x8B) return true;

    close (readFd);
    return false;
}

void sendFileToUnzip() {

}

















