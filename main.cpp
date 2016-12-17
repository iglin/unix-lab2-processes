#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <iostream>
#include <cmath>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>

using namespace std;

#define BUFSIZE 256
#define PORT 6969

pid_t p, q;

sem_t empty, full;

key_t shmkey;                 /*      shared memory key       */
int shmid;                    /*      shared memory id        */
sem_t *sem, *sem2;

int sock;
struct sockaddr_in addr;
int bytes_read, total = 0;

int main3() {
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(sock, (struct sockaddr *)&addr, sizeof(addr));
    char buf[BUFSIZE];
    while(1) {
        int socket = accept(sock, (struct sockaddr *)&addr, (socklen_t *) sizeof(addr));
        bytes_read = recvfrom(sock, buf, BUFSIZE, 0, NULL, NULL);
        total = total + bytes_read;
        buf[bytes_read] = '\0';
        fprintf(stdout, buf);fflush(stdout);
        if (total>BUFSIZE) break;
    }
    printf("\nTotal %d bytes received", total);
    return 0;
}

double calculate(char* par) {
    string input = string(par);
    char op = input[0];
    int i = 2;
    string s = "";
    while (input[i] != ';') {
        s += input[i];
        i++;
    }
    double a = stod(s);
    if (op == '+') {
        s = "";
        for (i++; input[i] != ';' && i < sizeof(input); i++) {
            s += input[i];
        }

        double b = stod(s);
        return a + b;
    }
    switch (op) {
        case 's':
            return a*a;
        case 'r':
            return sqrt(a);
    }
}

int main22() {
    char *s = (char *) "+;121;-55.7";

    printf("result %f\n", calculate(s));
    return 0;
}

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
        sem_wait(sem);
        char buf[BUFSIZE];
        double a, b;
        cout << "Input a: " << endl;
        cin >> a;
        cout << "Input b: " << endl;
        cin >> b;

        string s = "s;" + to_string(a);
        sem_post(sem2);

        //send

        sock = socket(AF_INET, SOCK_DGRAM, 0);
        addr.sin_family = AF_INET;
        addr.sin_port = htons(PORT);
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        cout << "parent socket: " << sock << endl;
        int ret = connect(sock, (struct sockaddr *)&addr, sizeof(addr));
        cout << "parent connected: " << ret << endl;
        send(sock, s.c_str(), strlen(s.c_str()), 0);
        close(sock);

        sem_wait(sem);
        cout << "parent is up to recieve" << endl;
      //  bind(sock, (struct sockaddr *)&addr, sizeof(addr));
        sock = socket(AF_INET, SOCK_DGRAM, 0);
        bind(sock, (struct sockaddr *)&addr, sizeof(addr));
        bytes_read = recvfrom(sock, buf, BUFSIZE, 0, NULL, NULL);
        cout << "parent recieved " << bytes_read << endl;
        buf[bytes_read] = '\0';
        cout << "parent got result " << buf << endl;

      //  cout << "sqr(a) = " << result << endl;

        sleep(5);
       // sem_post(sem2);
        kill(p, 9);
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
        sock = socket(AF_INET, SOCK_DGRAM, 0);
        addr.sin_family = AF_INET;
        addr.sin_port = htons(PORT);
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        bind(sock, (struct sockaddr *)&addr, sizeof(addr));
        char buf[BUFSIZE];
        while (1) {
            sem_wait(sem2);

            cout << "server in while" << endl;
           // int socket = accept(sock, (struct sockaddr *)&addr, (socklen_t *) sizeof(addr));
            //cout << "accepted: " << socket << endl;
            bytes_read = recvfrom(sock, buf, BUFSIZE, 0, NULL, NULL);
            cout << "read: " << socket << endl;
         //   total = total + bytes_read;
            buf[bytes_read] = '\0';
           // fprintf(stdout, buf);fflush(stdout);
         //   if (total>BUFSIZE) break;
            cout << "buf: " << buf << endl;
          //  printf("\nTotal %d bytes received\n", total);
            double res = calculate(buf);
            cout << "res: " << calculate(buf) << endl;
            close(sock);

            sem_post(sem);
            sock = socket(AF_INET, SOCK_DGRAM, 0);
            int ret = connect(sock, (struct sockaddr *)&addr, sizeof(addr));
            cout << "kid connected: " << ret << endl;
            const char *result = to_string(res).c_str();
            ret = send(sock, result, strlen(result), 0);
            cout << "kid sent result, " << ret << endl;
        }
        printf("Kid ends");
    }


    return 0;
}