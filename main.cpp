#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <fcntl.h>

pid_t p, q;

sem_t empty, full;

key_t shmkey;                 /*      shared memory key       */
int shmid;                    /*      shared memory id        */
sem_t *sem, *sem2;

int main() {
    char *e[]={"",""};

    /* initialize a shared variable in shared memory */
    shmkey = ftok ("/dev/null", 5);       /* valid directory name and a number */
    printf ("shmkey for p = %d\n", shmkey);
    shmid = shmget (shmkey, sizeof (int), 0644 | IPC_CREAT);
    if (shmid < 0){                           /* shared memory error check */
        perror ("shmget\n");
        exit (1);
    }

    sem = sem_open ("pSem", O_CREAT | O_EXCL, 0644, 1);
    sem2 = sem_open ("pSem2", O_CREAT | O_EXCL, 0644, 0);
    /* name of semaphore is "pSem", semaphore is reached using this name */
    sem_unlink ("pSem");
    sem_unlink ("pSem2");
    /* unlink prevents the semaphore existing forever */
    /* if a crash occurs during the execution         */
    printf ("semaphores initialized.\n\n");


    sem_init(&empty, 1, 1); // Установлен в 1
    sem_init(&full, 1, 0); // Установлен в 0

    p = fork(); /*Копирование адресного пространства и процесса*/
    if (p) {
        /* Родитель получает PID ребенка*/
        printf("Parent proc...\n");
        //wait(&q); /* Ждать завершения потомков */
        while(1) {
            sem_wait(sem);
            printf("parent!\n");
            sleep(5);
            sem_post(sem2);
        }

        printf("Parent ends");
        //shmdt (p);
        shmctl (shmid, IPC_RMID, 0);

        /* cleanup semaphores */
        sem_destroy (sem);
    }
    else
/* Ребенок получает 0 */
    {
        //   execv("./hello", e); /* Ребенок замещает себя другой программой*/
        printf("Kid proc...\n");
        while(1) {
            sem_wait(sem2);
            printf("kid!\n");
            sleep(5);
            sem_post(sem);
        }
        printf("Kid ends");
    }


    return 0;
}