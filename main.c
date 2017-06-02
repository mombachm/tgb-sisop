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
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <dirent.h>
#include <stdbool.h>



const char* DIRETORIO_SCAN = "./scan_files";
const int MAX_THREADS = 50;
pthread_t tid[50];
void* doSomeThing(void *arg);

int contThreads = 0;

int main(void)
{
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
                sendFileToUnzip(filepath);
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


int checkFileZip(char* filepath) {
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

void sendFileToUnzip(char* filepath) {
    int fd[2], nbytes;
    pid_t childpid;
    char string[] = "teste\n";
    char readbuffer[80];

    pipe(fd);

    if((childpid = fork()) == -1)
    {
        perror("fork");
        exit(1);
    }

    if(childpid == 0)
    {
            //Processo filho fecha a pipe de entrada

            unzipFile(filepath);
            //close(fd[0]);

            /* envia string pelo pipe */
            //write(fd[1], string, (strlen(string)+1));
            //exit(0);
    }
    else
    {
             //Processo pai fecha o pipe de saída
            //close(fd[1]);
            printf("processo pai");
            //teste de leitura de string do pipe
            //nbytes = read(fd[0], readbuffer, sizeof(readbuffer));
            //printf("String recebida: %s", readbuffer);
    }

}

void unzipFile(char* filepath) {
    int readFd;
    //Cria o descritor de arquivo de leitura
    readFd = open(filepath, O_RDONLY | O_CLOEXEC);
    if(readFd == -1){
        perror("open error");
        return;
    }
    printf("chamou unzip");
    execlp("gzip", "gzip", "-d", NULL);
}

void initNewThread() {
    int err;

    if (contThreads < MAX_THREADS)
    {
        err = pthread_create(&(tid[contThreads]), NULL, &doSomeThing, NULL);
        if (err != 0)
            printf("\ncan't create thread :[%s]", strerror(err));
        else
            printf("\n Thread %d criada.\n", contThreads);

        contThreads++;
    }
}

void* doSomeThing(void *arg)
{
    while(true) {
        write(1, "Teste", 5);
        sleep(1);
    }

    return NULL;
}


















