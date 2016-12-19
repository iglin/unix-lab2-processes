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

#define PAUSE 2

pid_t p;

sem_t *sem, *sem2;

int sock;
struct sockaddr_in addr;
int bytes_read;

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

void perform_action(char* buf, string action) {
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    sem_post(sem2);
    sleep(PAUSE);
    connect(sock, (struct sockaddr *)&addr, sizeof(addr));
    send(sock, action.c_str(), strlen(action.c_str()), 0);
    close(sock);

    sem_wait(sem);
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    bind(sock, (struct sockaddr *)&addr, sizeof(addr));
    bytes_read = recvfrom(sock, buf, BUFSIZE, 0, NULL, NULL);
    close(sock);
    buf[bytes_read] = '\0';
}

int main() {

    sem = sem_open ("pSem", O_CREAT | O_EXCL, 0644, 1);
    sem2 = sem_open ("pSem2", O_CREAT | O_EXCL, 0644, 0);

    sem_unlink ("pSem");
    sem_unlink ("pSem2");
    /* unlink prevents the semaphore existing forever */
    /* if a crash occurs during the execution         */
    printf ("semaphores initialized.\n\n");

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

        addr.sin_family = AF_INET;
        addr.sin_port = htons(PORT);
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

        string s = "s;" + to_string(a);
        perform_action(buf, s);
        string sqrA = buf;
        cout << "sqr(a) = " << sqrA << endl;

        s = "s;" + to_string(b);
        perform_action(buf, s);
        string sqrB = buf;
        cout << "sqr(b) = " << sqrB << endl;

        s = "+;" + sqrA + ";" + sqrB;
        perform_action(buf, s);
        cout << "sqr(a) + sqr(b) = " << buf << endl;

        s = "r;" + string(buf);
        perform_action(buf, s);
        cout << "sqrt [ sqr(a) + sqr(b) ] = " << buf << endl;

        kill(p, 9);

        /* cleanup semaphores */
        sem_destroy (sem);
        sem_destroy (sem2);
    }
    else
/* Ребенок получает 0 */
    {
        addr.sin_family = AF_INET;
        addr.sin_port = htons(PORT);
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        char buf[BUFSIZE];
        while (1) {
            sock = socket(AF_INET, SOCK_DGRAM, 0);
            bind(sock, (struct sockaddr *)&addr, sizeof(addr));

            sem_wait(sem2);
            bytes_read = recvfrom(sock, buf, BUFSIZE, 0, NULL, NULL);
            cout << "kid read " << bytes_read << " bytes" << endl;
            close(sock);
            buf[bytes_read] = '\0';
            double res = calculate(buf);

            sem_post(sem);
            sock = socket(AF_INET, SOCK_DGRAM, 0);
            sleep(PAUSE);
            int ret = connect(sock, (struct sockaddr *)&addr, sizeof(addr));
            cout << "kid connected: " << ret << endl;
            const char *result = to_string(res).c_str();
            ret = send(sock, result, strlen(result), 0);
            close(sock);
            cout << "kid sent result, " << ret << " bytes" << endl;
        }
    }


    return 0;
}