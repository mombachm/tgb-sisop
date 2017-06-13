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

    //writeDebugOutput();
    writeResultsOutput();

    for(int i=0; i<MAX_INFECTED_FILES; i++){
        free(infectedFiles[i]);
    }

    //initNewThread();
    return(EXIT_SUCCESS);
}

void writeResultsOutput() {
    //put infected files on standard output
    for(int i=0; i<MAX_INFECTED_FILES; i++){
        if(infectedFiles[i] != NULL) {
            totalInfected++;
            fprintf(stdout,"%s\n", infectedFiles[i]->filename);
        }
    }

    fprintf(stderr, "%d/%d", totalInfected, totalVerified);
}

void init() {
    for(int i=0; i<MAX_INFECTED_FILES; i++){
        infectedFiles[i] = NULL;
    }
    pthread_mutex_init(&mutex, NULL);
}

void writeDebugOutput() {
    printf("\n****** Infected Files ******\n");
    for(int i=0; i<MAX_INFECTED_FILES; i++){
        if(infectedFiles[i] != NULL) {
            totalInfected++;
            printf( "%s      -  %d\n", infectedFiles[i]->filename);
        }
    }
    printf("\n****************************\n");
    printf("\n****** Virus Results ******\n");
    printf("\nSearched virus signature: %s", virusAss);
    printf("\nTotal verified files: %d \\ Total infected files: %d\n", totalVerified, totalInfected);
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
            initThreadVerifyVirus(readFd, filepaths[i]);
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

    //Create file descriptor to read the file and check if is compressed
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
        initThreadVerifyVirus(pipeGzip[0], filepath);
    }

}


void unzipFile(char* filepath) {
    //Create the read file descriptor
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

void initThreadVerifyVirus(char *readFd, char *filename) {
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

        infectedFiles[contThreads] = malloc(sizeof(VirusFile));
        infectedFiles[contThreads]->filename = filename;

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


    //Receive the read file descriptor
    //Create a buffer to test
    char readbuffer[1000];
    if(readFd == -1){
        perror("open error");
        return;
    }

    //sleep(2);
    ssize_t res = read (readFd, readbuffer, 1000);

    //printf("\n\n%s\n\n", readbuffer);

    void *find;
    bool found = false;
    find = memmem(readbuffer, 1000, virusAss, strlen(virusAss));
    if (find) {
        found = true;
    }
    //IMPLEMENT SEARCH FOR VIRUS SIGNATURE
    pthread_mutex_lock (&mutex);

    totalVerified++;
    pthread_t self;
    self = pthread_self();
    int indexThread = -1;
    for(int i = 0; i < contThreads; i++) {
        if(pthread_equal(tid[i], self))
            indexThread = i;
    }
    if(indexThread >= 0) {
        if(!found) {
            infectedFiles[indexThread] = NULL;
        }
    }

    pthread_mutex_unlock (&mutex);
    close (readFd);
    pthread_exit((void*) 0);
}

















