/*
 ============================================================================
 Name        : main.c
 Author      : Felipe Flores and Matheus Mombach
 Version     :
 Copyright   :
 Description : Trab GB - Virus Scanner
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
#include <stdbool.h>

#define MAX_THREADS 50
#define MAX_INFECTED_FILES 50
#define MAX_FILESIZE 1000000
#define MAX_FILENAME_SIZE 100

pthread_t tid[MAX_THREADS]; //array of thread IDs
pthread_mutex_t mutex; //mutex used in the threads function

int contThreads = 0;//counter for threads created
int pipeGzip[2];    //pipe to get the output of gzip command in the parent process
char* virusAss;     //variable to store the virus signature
char* infectedFiles[MAX_INFECTED_FILES]; //array to store the filename of infected files
int totalInfected = 0;  //counter of infected files
int totalVerified = 0;  //counter of files checked

void* virusVerification(void *arg);
void writeResultsOutput();
void writeDebugOutput();
void checkFilesForVirus(int len, char **filepaths);
int checkFileCompressed(char* filepath);
void sendFileToDecompress(char* filepath);
void decompressFile(char* filepath);
void initThreadVerifyVirus(char *readFd, char *filename);
void waitThreads();
void* virusVerification(void* readFd);

/**
FUNCTION:       void main()
---------------------------------
DESCRIPTION:    It executes the main routine
*/
int main(int argc, char **argv){
    init(); //call init function to initialize general configurations
    if(argc > 1) virusAss = argv[1]; //get virus signature
    checkFilesForVirus(argc, argv); //call function to read all files and check for viruses

    //writeDebugOutput(); //Debug output
    writeResultsOutput(); //SDT_OUT and STD_ERR output

    for(int i=0; i<MAX_INFECTED_FILES; i++){
        free(infectedFiles[i]);
    }
    return(EXIT_SUCCESS);
}

/**
FUNCTION:       void writeResultsOutput()
---------------------------------
DESCRIPTION:    This function write the results on the standard outputs
*/
void writeResultsOutput() {
    //put infected files on standard output
    for(int i=0; i<MAX_INFECTED_FILES; i++){
        if(infectedFiles[i] != NULL) {
            fprintf(stdout,"%s\n", infectedFiles[i]);
        }
    }

    fprintf(stderr, "%d/%d\n", totalInfected, totalVerified); //put (total infected / total verified) on standard error output
}

/**
FUNCTION:       void init()
---------------------------------
DESCRIPTION:    This function initialize variables and configurations
*/
void init() {
    for(int i=0; i<MAX_INFECTED_FILES; i++){
        infectedFiles[i] = NULL;
    }
    pthread_mutex_init(&mutex, NULL);
}

/**
FUNCTION:       void writeDebugOutput()
---------------------------------
DESCRIPTION:    This function shows a custom debug on standard output
*/
void writeDebugOutput() {
    printf("\n****** Virus Results ******\n");
    printf("\nSearched virus signature: %s", virusAss);
    printf("\n****** Infected Files ******\n");
    printf("_________________________\n");
    for(int i=0; i<MAX_INFECTED_FILES; i++){
        if(infectedFiles[i] != NULL) {
            totalInfected++;
            printf( "%s\n", infectedFiles[i]);
        }
    }
    printf("_________________________\n");
    printf("\nTotal verified files: %d \\ Total infected files: %d\n", totalVerified, totalInfected);
    printf("\n****************************\n");
}

/**
FUNCTION:       checkFilesForVirus(int len, char **filepaths)
---------------------------------
DESCRIPTION:    This function iterates in all files of 'filepaths' and call the function to verify if it's compressed.
If the file is compressed, it calls the function to decompress the file. The function to check for the virus is called directly
for the files which aren't compressed.

---------------------------------
INPUT:  int len = number of files in the array
        char **filepaths = pointer for the array of files
*/
void checkFilesForVirus(int len, char **filepaths) {
    int readFd;
    for( int i=2; i<len; i++ ) {
        if(checkFileCompressed(filepaths[i]) == true) {
            sendFileToDecompress(filepaths[i]); //decompress file
        } else {
            readFd = open(filepaths[i], O_RDONLY | O_CLOEXEC, S_IRUSR | S_IWUSR );
            if(readFd == -1){
                perror("open error");
            }
            initThreadVerifyVirus(readFd, filepaths[i]); //init virus verification
        }
    }

    waitThreads();  //wait for all threads to finish
    pthread_mutex_destroy(&mutex);
    return infectedFiles;
}

/**
FUNCTION:       checkFilesForVirus(int len, char **filepaths)
---------------------------------
DESCRIPTION:    This function check if the file passed as parameter is compressed or not.
The function return a value representing boolean values (true if the file is compressed, otherwise returns false).
---------------------------------
INPUT:  char *filepaths = pointer for the file path string
OUTPUT: int (true if the file is compressed, otherwise returns false)
*/
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
    if (readBuffer[0]==0x1F && readBuffer[1]==0x8B) return true; //check the first two bytes of the file to verify if is compressed
    close (readFd);
    return false;
}

/**
FUNCTION:       sendFileToDecompress(char* filepath)
---------------------------------
DESCRIPTION:    This function creates a new process to decompress the file. The child process created
calls the decompress function. The output of the file decompressed is passed by a pipe
to the parent process. When the file is decompressed, another function is called to check for the virus.
---------------------------------
INPUT:  char *filepath = pointer for file path to decompress
*/
void sendFileToDecompress(char* filepath) {
    pid_t childpid;
    pipe(pipeGzip); //create pipe for get the file decompressed

    if((childpid = fork()) == -1){ //fork error
        perror("fork");
        exit(EXIT_FAILURE);
    }
    if(childpid == 0){ //child
        decompressFile(filepath);
        exit(EXIT_FAILURE);
    } else { //parent
        close(pipeGzip[1]);
        initThreadVerifyVirus(pipeGzip[0], filepath); //call virus verification
    }
}

/**
FUNCTION:       decompressFile(char* filepath)
---------------------------------
DESCRIPTION:    This function creates a file descriptor for the file path passed as parameter
and connect a global pipe to redirect the readed file content to STDIN to be used by gzip.
With the writing side of the pipe is sended the file decompressed. The gzip is executed with execlp.
---------------------------------
INPUT:  char *filepath = pointer for file path to decompress
*/
void decompressFile(char* filepath) {
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

    execlp("gzip", "gzip", "-d", NULL);
    exit(EXIT_FAILURE);
}

/**
FUNCTION:       initThreadVerifyVirus(char *readFd, char *filename)
---------------------------------
DESCRIPTION:    This function creates a thread to verify the virus in the file passed as parameter.
The threads number are limited by the MAX_THREADS define. The array of infected files is attached
with the thread id.
---------------------------------
INPUT:  char *readFd = pointer for the file descriptor
        char *filename = pointer for file path
*/
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

        infectedFiles[contThreads] = strdup(filename); //strdup returns a pointer to a new allocated string duplicating the string passed as parameter
        contThreads++;
    }
    pthread_attr_destroy(&attr);
}

/**
FUNCTION:       waitThreads()
---------------------------------
DESCRIPTION:    This function call the join function of the threads to wait all threads to terminate
*/
void waitThreads() {
    int err;
    for(int i=0; i<contThreads; i++){
        err = pthread_join(tid[i], NULL);
        if (err) {
            perror("pthread_join()");
            exit(EXIT_FAILURE);
        }
    }
}

/**
FUNCTION:       virusVerification(void* readFd)
---------------------------------
DESCRIPTION:    This function is executed by the threads. It reads the file by the file descriptor
and search for the virus signature. It is used a mutex to modify the shared memory.
The files which aren't infected are removed from the infectedFiles array.
---------------------------------
INPUT:  char *readFd = pointer for the file descriptor
*/
void* virusVerification(void* readFd){
    //Receive the read file descriptor
    //Create a buffer to read the file for verify the virus
    char readbuffer[MAX_FILESIZE];
    if(readFd == -1){
        perror("open error");
        return;
    }

    ssize_t res = read (readFd, readbuffer, MAX_FILESIZE);

    void *find;
    bool found = false;
    find = memmem(readbuffer, MAX_FILESIZE, virusAss, strlen(virusAss));
    if (find) {
        found = true;
        totalInfected++;
    }

    //It's used mutex to access the shared memory
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
    pthread_exit(NULL);
}














