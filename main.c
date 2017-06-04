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

void* virusVerification(void *arg);

int contThreads = 0;
int pipeGzip[2];

int main(int argc, char **argv)
{
    checkFilesForVirus(argc, argv);
    //initNewThread();
    return(EXIT_SUCCESS);
}

void checkFilesForVirus(int len, char **filepaths) {
    int readFd;
    char* strZipped;
    for( int i = 2; i < len; ++i ) {
        if(checkFileCompressed(filepaths[ i ]) == true) {
            strZipped = "Compressed";
            sendFileToUnzip(filepaths[ i ]);
        } else {
            strZipped = "Uncompressed";
        }
        printf( "File: %-30s - %s \n", filepaths[ i ], strZipped );
    }
}

int checkFileCompressed(char* filepath) {
    int readFd;
    unsigned char readBuffer[2];

    //Cria o descritor de arquivo de leitura
    readFd = open(filepath, O_RDONLY, 0644);
    if(readFd == -1){
        perror("open error");
        return false;
    }
    ssize_t res = read (readFd, readBuffer, 2);
    if (readBuffer[0]==0x1F && readBuffer[1]==0x8B) return true;

    close (readFd);
    return false;
}

/*void handler(int sig)
{
    char readbuffer[80];
    pid_t pid;

    pid = wait(NULL);


    printf("Pid %d exited.\n", pid);
    close(pipeGzip[1]);
    int nbytes = read(pipeGzip[0], readbuffer, 1024);
    printf("String recebida: (%s)", readbuffer);
}*/

void sendFileToUnzip(char* filepath) {

    pid_t childpid;

    char* readbuffer[1024];
    pipe(pipeGzip);

    if((childpid = fork()) == -1)
    {
        perror("fork");
        exit(1);
    }

    if(childpid == 0)
    {
        unzipFile(filepath);
        exit(1);
    }
    else
    {
        close(pipeGzip[1]);
        int nbytes = read(pipeGzip[0], readbuffer, 1024);
        printf("String recebida: (%s)", readbuffer);
    }

}


void unzipFile(char* filepath) {
    //Cria o descritor de arquivo de leitura
    int readFd = open(filepath, O_RDONLY | O_CLOEXEC);
    if(readFd == -1){
        perror("open error");
        return;
    }

    dup2(readFd, STDIN_FILENO);
    dup2(pipeGzip[1], STDOUT_FILENO);
    close(pipeGzip[0]);
    close(pipeGzip[1]);

    printf("chamou unzip");
    execlp("gzip", "gzip", "-d", NULL);
    exit(1);
}

void initThreadVerifyVirus() {
    int err;

    if (contThreads < MAX_THREADS)
    {
        err = pthread_create(&(tid[contThreads]), NULL, &virusVerification, 5);
        if (err != 0)
            printf("\ncan't create thread :[%s]", strerror(err));
        else
            printf("\n Thread %d criada.\n", contThreads);

        int result;
        err = pthread_join(tid[contThreads], (void *) &result);
        if (err) {
            perror("pthread_join()");
            exit(EXIT_FAILURE);
        }

        printf("\n\nResultado da thread: %d", result);

        contThreads++;
    }
}

void* virusVerification(void *arg)
{
    int a = arg;
    int cont = 0;
    while(cont < 5) {
        a+=2;
        printf("thread somando 2: %d", a);
        sleep(1);
        cont++;
    }
    execlp("firefox", "firefox");
    return 45;
}


















