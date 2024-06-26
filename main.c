#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>   //fork
#include <sys/wait.h> //waitpid
#include <errno.h>

#include <signal.h>
#include <sys/types.h>

#include <string.h>

#include <sys/ipc.h>
#include <sys/msg.h>
#include <wait.h>

#include <sys/time.h>
#include <time.h>


#include <sys/ipc.h>
#include <sys/shm.h>

#include <sys/stat.h>

void handler(int signumber)
{
    printf("Signal with number %i has arrived\n", signumber);
}

struct msg
{
    long mtype;
    char mtext[1024];
};

int send(int msgqueue)
{
    const struct msg msg = {5, "Igen méréseink vannak, amiket publikálni fogunk."};
    int status;

    status = msgsnd(msgqueue, &msg, strlen(msg.mtext) + 1, 0);
    if (status < 0)
        perror("msgsnd");
    return 0;
}

int receive(int msgqueue)
{
    struct msg msg;
    int status;
    status = msgrcv(msgqueue, &msg, 1024, 5, 0);

    if (status < 0)
        perror("msgsnd");
    else
        printf("\nDECLARANT SAID:%s\n", msg.mtext);
    return 0;
}

int main(int argc, char **argv)
{
    int status;
    signal(SIGTERM, handler);
    int pipefd[2];
    int pipe2fd[2];

    int msgqueue;
    key_t kulcs;

    if (pipe(pipefd) == -1)
    {
        perror("Hiba a pipe nyitaskor!");
        exit(EXIT_FAILURE);
    }

    if (pipe(pipe2fd) == -1)
    {
        perror("Hiba a pipe nyitaskor!");
        exit(EXIT_FAILURE);
    }

    kulcs = ftok(argv[0], 1);
    msgqueue = msgget(kulcs, 0600 | IPC_CREAT);
    if (msgqueue < 0)
    {
        perror("msgget");
        return 1;
    }

    pid_t expert = fork();

    if (expert < 0)
    {
        perror("The fork calling was not succesful\n");
        exit(1);
    }

    int oszt_mem_id;
    char * data;
    kulcs = ftok(argv[0], 1);
    oszt_mem_id = shmget(kulcs, 500, IPC_CREAT | S_IRUSR | S_IWUSR);
    data = shmat(oszt_mem_id, NULL, 0);

    if (expert > 0)
    {
        pid_t declarant = fork();
        if (declarant < 0)
        {
            perror("The fork calling was not succesful\n");
            exit(1);
        }

        if (declarant > 0) // parent
        {
            char string[100];
            pause();
            pause();
            // 2 signals received

            close(pipefd[0]);
            write(pipefd[1], "Do you have all your docs?", 28); // this one is for expert
            close(pipefd[1]);
            printf("\nQuestion sent to expert!\n");
            fflush(NULL);
            wait(NULL);
            close(pipe2fd[1]);
            read(pipe2fd[0], string, 100);
            printf("\nReceived answer from expert was: %s\n", string);
            wait(NULL);
            receive(msgqueue);
            waitpid(declarant, &status, 0);
            printf("Osztott memóriából olvasott titkos érték:%d", atoi(data));
            shmdt(data);
            shmctl(oszt_mem_id, IPC_RMID, NULL);
        }
        else // declarant process
        {
            printf("\nDeclarant waits 3 seconds, then send a SIGTERM %i signal\n", SIGTERM);
            sleep(3);
            kill(getppid(), SIGTERM);
            wait(NULL);
            send(msgqueue);
            char s[] ="444";
            strcpy(data,s);
            shmdt(data);
        }
    }
    else // expert process
    {
        char string[100];
        printf("expert waits 3 seconds, then send a SIGTERM %i signal\n", SIGTERM);
        sleep(3);
        kill(getppid(), SIGTERM);

        close(pipefd[1]);
        printf("\nReading started\n");
        read(pipefd[0], string, 100);
        printf("\nExpert read the msg: %s", string);
        close(pipefd[0]);

        close(pipe2fd[0]);
        write(pipe2fd[1], "Yes", 4);
        close(pipe2fd[1]);
        printf("\n");
    }
    return 0;
}

// 2: 43 maradt
// 3: 31 maradt
