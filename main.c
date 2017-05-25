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
      /* print all the files and directories within directory */
      while ((ent = readdir (dir)) != NULL) {

        char filepath[1024];
        if(strcmp(ent->d_name,".")!=0 && strcmp(ent->d_name,"..")!=0) {
            sprintf(filepath, "%s/%s", DIRETORIO_SCAN, ent->d_name);
            printf ("\n%s\n", /*ent->d_name*/ filepath);
            checkFileZip(filepath);
        }
      }
      closedir (dir);
    } else {
      /* could not open directory */
      perror ("");
      return EXIT_FAILURE;
    }
}


void checkFileZip(char* filepath) {
        int input_fd, output_fd;    /* Input and output file descriptors */
        ssize_t ret_in, ret_out;    /* Number of bytes returned by read() and write() */


        /* Create output file descriptor */
        output_fd = open(filepath, O_RDONLY, 0644);
        if(output_fd == -1){
            perror("open error");
            return 3;
        }


        char buffer[2];

        //write (output_fd, "teste", strlen("teste"));

        int res = read (output_fd, buffer, 2);

        printf("\n%02x", buffer[1]);


        /* Create input file descriptor */
        /*input_fd = open (ARQUIVO_FIXO, O_RDONLY);
        if (input_fd == -1) {
            perror ("open error");
            return 2;
        }

        /* Close file descriptors */
        //close (input_fd);
        close (output_fd);

}



















