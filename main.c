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


#define MAX_THREADS 50
#define MAX_INFECTED_FILES 50
#define MAX_FILENAME_SIZE 50


typedef struct
{
    int nFound;
    char *filename;
} VirusFile;

pthread_t tid[MAX_THREADS];
pthread_mutex_t mutex;

void* virusVerification(void *arg);
void* testFile(int readFd);

int contThreads = 0;
int pipeGzip[2];

char* virusAss;

VirusFile* infectedFiles[MAX_INFECTED_FILES];
int totalInfected = 0;
int totalVerified = 0;

int main(int argc, char **argv)
{
    init();
    if(argc > 1) virusAss = argv[1];
    checkFilesForVirus(argc, argv);

    writeDebugOutput();
    writeResultsOutput();

    for(int i=0; i<MAX_INFECTED_FILES; i++){
        free(infectedFiles[i]);
    }

    //initNewThread();
    return(EXIT_SUCCESS);
}

void writeResultsOutput() {
    //put total infected and total verified files on standard error output
    fprintf(stderr, "%d/%d", totalInfected, totalVerified);
    //put infected files on standard output
    for(int i=0; i<MAX_INFECTED_FILES; i++){
        if(infectedFiles[i] != NULL) {
            fprintf(stdout,"%s\n", infectedFiles[i]->filename);
        }
    }
}

void init() {
    //memset(infectedFiles, NULL, MAX_INFECTED_FILES * sizeof *infectedFiles);
    for(int i=0; i<MAX_INFECTED_FILES; i++){
        infectedFiles[i] = NULL;
    }
    pthread_mutex_init(&mutex, NULL);
}

void writeDebugOutput() {
    printf("\n****** Virus Results ******\n");
    printf("\nSearched virus signature: %s", virusAss);
    printf("\nTotal verified files: %d \\ Total infected files: %d\n", totalVerified, totalInfected);
    printf("\n****** Infected Files ******\n");
    for(int i=0; i<MAX_INFECTED_FILES; i++){
        if(infectedFiles[i] != NULL) {
            printf( "%s\n", infectedFiles[i]->filename);
        }
    }
    printf("\n****************************\n");
}

void checkFilesForVirus(int len, char **filepaths) {
    int readFd;
    char* strZipped;
    for( int i=2; i<len; i++ ) {
        if(checkFileCompressed(filepaths[i]) == true) {
            strZipped = "Compressed";
            sendFileToUnzip(filepaths[i]);
        } else {
            strZipped = "Uncompressed";
            readFd = open(filepaths[i], O_RDONLY | O_CLOEXEC, S_IRUSR | S_IWUSR );
            if(readFd == -1){
                perror("open error");
            }
            initThreadVerifyVirus(readFd);
        }
        //custom printf for debug
        //printf( "File: %-30s - %s \n", filepaths[ i ], strZipped );
    }


    aguardaThreads();
    pthread_mutex_destroy(&mutex);
    return infectedFiles;
}

int checkFileCompressed(char* filepath) {
    int readFd;
    unsigned char readBuffer[2];

    //create file descriptor to read the file and check if is compressed
    readFd = open(filepath, O_RDONLY | O_CLOEXEC, S_IRUSR | S_IWUSR );
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
        initThreadVerifyVirus(pipeGzip[0]);
        //int nbytes = read(pipeGzip[0], readbuffer, 1024);
        //printf("String recebida: (%s)", readbuffer);
        //int readFd = open(readbuffer, O_RDONLY | O_CLOEXEC);

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

void initThreadVerifyVirus(char* readFd) {
    int err;
    pthread_attr_t attr;
    if (contThreads < MAX_THREADS)
    {
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
        err = pthread_create(&(tid[contThreads]), &attr, &virusVerification, (void *)readFd);
        if (err != 0) {
            perror("pthread_create()");
        }
        contThreads++;
    }
    pthread_attr_destroy(&attr);
}

void aguardaThreads() {
    int err;
    void *status;
    for(int i=0; i<contThreads; i++){
        err = pthread_join(tid[i], (void *) &status);
        if (err) {
            perror("pthread_join()");
            exit(EXIT_FAILURE);
        }
    }
}

void* virusVerification(void* readFd)
{
    //Cria o descritor de arquivo de leitura
    char readbuffer[20];
    if(readFd == -1){
        perror("open error");
        return;
    }

    //sleep(2);
    ssize_t res = read (readFd, readbuffer, 20);

    //IMPLEMENTAR BUSCA DA ASSINATURA DO VIRUS
    pthread_mutex_lock (&mutex);
    totalVerified++;
    int i = 0;
    while(&infectedFiles[i] != NULL && i <= contThreads) {
        infectedFiles[i] = malloc(sizeof(VirusFile));
        infectedFiles[i]->filename = "teste";
        infectedFiles[i]->nFound = i;
        i++;
    }
    pthread_mutex_unlock (&mutex);
    close (readFd);
    pthread_exit((void*) 0);
}

















