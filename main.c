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

#define O_RDONLY         00
#define O_WRONLY         01
#define O_RDWR           02

const char* ARQUIVO_FIXO = "./teste";


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






        int input_fd, output_fd;    /* Input and output file descriptors */
        ssize_t ret_in, ret_out;    /* Number of bytes returned by read() and write() */


        /* Create output file descriptor */
        output_fd = open(ARQUIVO_FIXO, O_RDWR, 0644);
        if(output_fd == -1){
            perror("open error");
            return 3;
        }


        char buffer[strlen("teste")];

        //write (output_fd, "teste", strlen("teste"));

        int res = read (output_fd, buffer, 3);

        printf("\n%d", res);
        printf("\n%s", buffer);


        /* Create input file descriptor */
        /*input_fd = open (ARQUIVO_FIXO, O_RDONLY);
        if (input_fd == -1) {
            perror ("open error");
            return 2;
        }



        printf(read(input_fd));*/


        /* Copy process */



        /* Close file descriptors */
        close (input_fd);
        close (output_fd);



        return(0);
}


























