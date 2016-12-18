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

#define PAUSE 5

pid_t p, q;

sem_t empty, full;

key_t shmkey;                 /*      shared memory key       */
int shmid;                    /*      shared memory id        */
sem_t *sem, *sem2;

int sock;
struct sockaddr_in addr;
int bytes_read, total = 0;

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

int main() {
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

    sem_unlink ("pSem");
    sem_unlink ("pSem2");
    /* unlink prevents the semaphore existing forever */
    /* if a crash occurs during the execution         */
    printf ("semaphores initialized.\n\n");


    sem_init(&empty, 1, 1); // Установлен в 1
    sem_init(&full, 1, 0); // Установлен в 0

    //char *e[]={"",""};
    p = fork(); /*Копирование адресного пространства и процесса*/
    if (p) {
        /* Родитель получает PID ребенка*/
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
        sleep(PAUSE);

        sock = socket(AF_INET, SOCK_DGRAM, 0);
        addr.sin_family = AF_INET;
        addr.sin_port = htons(PORT);
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        connect(sock, (struct sockaddr *)&addr, sizeof(addr));
        send(sock, s.c_str(), strlen(s.c_str()), 0);
        close(sock);

        sem_wait(sem);
        sock = socket(AF_INET, SOCK_DGRAM, 0);
        bind(sock, (struct sockaddr *)&addr, sizeof(addr));
        bytes_read = recvfrom(sock, buf, BUFSIZE, 0, NULL, NULL);
        close(sock);
        buf[bytes_read] = '\0';
        string sqrA = buf;
        cout << "sqr(a) = " << sqrA << endl;
        s = "s;" + to_string(b);

        sock = socket(AF_INET, SOCK_DGRAM, 0);
        sem_post(sem2);
        sleep(PAUSE);
        connect(sock, (struct sockaddr *)&addr, sizeof(addr));
        int ret = send(sock, s.c_str(), strlen(s.c_str()), 0);
        cout << "parent sent " << ret << endl;
        close(sock);

        sem_wait(sem);
        sock = socket(AF_INET, SOCK_DGRAM, 0);
        bind(sock, (struct sockaddr *)&addr, sizeof(addr));
        bytes_read = recvfrom(sock, buf, BUFSIZE, 0, NULL, NULL);
        close(sock);
        buf[bytes_read] = '\0';
        string sqrB = buf;
        cout << "sqr(b) = " << sqrB << endl;
        s = "+;" + sqrA + ";" + sqrB;

        sock = socket(AF_INET, SOCK_DGRAM, 0);
        sem_post(sem2);
        sleep(PAUSE);
        connect(sock, (struct sockaddr *)&addr, sizeof(addr));
        send(sock, s.c_str(), strlen(s.c_str()), 0);
        close(sock);

        sem_wait(sem);
        sock = socket(AF_INET, SOCK_DGRAM, 0);
        bind(sock, (struct sockaddr *)&addr, sizeof(addr));
        bytes_read = recvfrom(sock, buf, BUFSIZE, 0, NULL, NULL);
        close(sock);
        buf[bytes_read] = '\0';
        cout << "sqr(a) + sqr(b) = " << buf << endl;
        s = "r;" + string(buf);

        sock = socket(AF_INET, SOCK_DGRAM, 0);
        sem_post(sem2);
        sleep(PAUSE);
        connect(sock, (struct sockaddr *)&addr, sizeof(addr));
        send(sock, s.c_str(), strlen(s.c_str()), 0);
        close(sock);

        sem_wait(sem);
        sock = socket(AF_INET, SOCK_DGRAM, 0);
        bind(sock, (struct sockaddr *)&addr, sizeof(addr));
        bytes_read = recvfrom(sock, buf, BUFSIZE, 0, NULL, NULL);
        close(sock);
        buf[bytes_read] = '\0';
        cout << "sqrt [ sqr(a) + sqr(b) ] = " << buf << endl;

        kill(p, 9);
        shmctl (shmid, IPC_RMID, 0);

        /* cleanup semaphores */
        sem_destroy (sem);
        sem_destroy (sem2);
    }
    else
/* Ребенок получает 0 */
    {
        //   execv("./hello", e); /* Ребенок замещает себя другой программой*/
        printf("Kid proc...\n");
        addr.sin_family = AF_INET;
        addr.sin_port = htons(PORT);
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        char buf[BUFSIZE];
        while (1) {
            sock = socket(AF_INET, SOCK_DGRAM, 0);
            bind(sock, (struct sockaddr *)&addr, sizeof(addr));

            sem_wait(sem2);
            cout << "kid got sem2 " << endl;
            bytes_read = recvfrom(sock, buf, BUFSIZE, 0, NULL, NULL);
            cout << "kid read " << bytes_read << endl;
            close(sock);
            buf[bytes_read] = '\0';
           // fprintf(stdout, buf);fflush(stdout);
         //   if (total>BUFSIZE) break;
          //  printf("\nTotal %d bytes received\n", total);
            double res = calculate(buf);

            sem_post(sem);
            sock = socket(AF_INET, SOCK_DGRAM, 0);
            sleep(PAUSE);
            int ret = connect(sock, (struct sockaddr *)&addr, sizeof(addr));
            cout << "kid connected: " << ret << endl;
            const char *result = to_string(res).c_str();
            ret = send(sock, result, strlen(result), 0);
            close(sock);
            cout << "kid sent result, " << ret << endl;
        }
        printf("Kid ends");
    }


    return 0;
}