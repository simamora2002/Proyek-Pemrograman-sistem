#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>

#define WORD_CNT 10
#define BUFSIZE 100

void error_handling(char *message);
void z_handler(int sig);

int main(int argc, char *argv[])
{
    /* Variabel untuk kuis */
    char word_bank[WORD_CNT][BUFSIZE] = {"processor", "memory", "network", "port", "binary", "output", "input", "motherboard", "encrypt", "router"}; // variable untuk menyimpan string yang akan dicocokan
    char q_word[BUFSIZE];                                                                          // variabel untuk menyimpan string yang diambil dari word store
    char print_str[BUFSIZE];                                                                       // variable yang menunjukkan kata-kata yang cocok sejauh ini

    int try_cnt = 0; // jumlah upaya
    int i, j;
    int mistake = 0;

    /* Variabel untuk server dan klien */
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_addr, clnt_addr;

    pid_t pid;
    struct sigaction act;
    int str_len, state, addr_size;

    // variable untuk komunikasi IPC
    int fd1[2], fd2[2];
    char buffer[BUFSIZE];

    srand(time(NULL));

    if (argc != 2)
    {
        printf("Usage : %s <port>\n", argv[0]);
        exit(1);
    }

    if (pipe(fd1) < 0 || pipe(fd2) < 0)
        error_handling("Pipe() error!!");

    act.sa_handler = z_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;

    state = sigaction(SIGCHLD, &act, 0);
    if (state != 0)
        error_handling("sigaction() error");

    serv_sock = socket(PF_INET, SOCK_STREAM, 0);

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(argv[1]));

    if (bind(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
        error_handling("bind() error");

    if (listen(serv_sock, 5) == -1)
        error_handling("listen() error");

    // server selalu aktif 
    while (true)
    {
        addr_size = sizeof(clnt_addr);
        clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_addr, &addr_size);
        if (clnt_sock == -1)
            continue;

        pid = fork();
        if (pid == -1)
        {
            close(clnt_sock);
            continue;
        }
        else if (pid > 0) // parent process
        {
            close(clnt_sock);

            int index = rand() % WORD_CNT;
            int word_len = 0;
            char clnt_str[BUFSIZE]; // variable untuk menerima jawaban yang benar dimasukkan oleh klien
            char send_str[BUFSIZE]; // Variabel untuk pemformatan

            strcpy(q_word, word_bank[index]);

            for (i = 0; i < strlen(q_word); i++)
                print_str[i] = '_';

            print_str[strlen(q_word)] = '\0';

            word_len = strlen(q_word);

            printf("correct answer string : %s\n", q_word);

            write(fd1[1], &word_len, sizeof(int)); // lulus jumlah karakter

            try_cnt = 0;
            mistake = 0;
            while (true)
            {
                read(fd2[0], clnt_str, BUFSIZE);

                // saat memasukkan kata
                if (strlen(clnt_str) == 1)
                {
                    int flag = false;
                    for (j = 0; j < strlen(q_word); j++)
                        if (clnt_str[0] == q_word[j])
                        {
                            print_str[j] = clnt_str[0];
                            flag = true;
                        }
                    if(flag == false)
                        write(fd1[1], "There is no such character in the quiz. Try another letter!", BUFSIZE);
                        mistake++;
                }
                else
                {
                    if (strcmp(clnt_str, q_word) == 0)
                        strcpy(print_str, clnt_str);
                    else
                        write(fd1[1], "Incorrect answer!. Try another word!", BUFSIZE);
                        mistake++;
                }
                try_cnt++;

                if (strcmp(q_word, print_str) == 0)
                {
                    write(fd1[1], "Congratulations! That's the answer!", BUFSIZE);
                    sprintf(send_str, "guessed word : %s", print_str);
                    write(fd1[1], send_str, BUFSIZE);
                    sprintf(send_str, "number of attempts : %d", try_cnt);
                    write(fd1[1], send_str, BUFSIZE);
                    sprintf(send_str, "mistake : %d", mistake);
                    write(fd1[1], send_str, BUFSIZE);
                    write(fd1[1], "close the program.", BUFSIZE);
                    write(fd1[1], "", BUFSIZE);
                    break;
                }else if (mistake == 6)
                {
                    write(fd1[1], "Sorry you're out of turns", BUFSIZE);
                    sprintf(send_str, "correct answer string : %s", q_word);
                    write(fd1[1], send_str, BUFSIZE);
                    write(fd1[1], "close the program.", BUFSIZE);
                    write(fd1[1], "", BUFSIZE);
                    break;
                }

                sprintf(send_str, "hint : %s", print_str);
                write(fd1[1], send_str, BUFSIZE);
                write(fd1[1], "", BUFSIZE);
            }
        }
        else // child process
        {
            close(serv_sock);
            bool flag = true;

            read(fd1[0], buffer, sizeof(int));
            write(clnt_sock, buffer, sizeof(int));

            while (true)
            {
                read(clnt_sock, buffer, BUFSIZE);
                write(fd2[1], buffer, BUFSIZE);

                while (true)
                {
                    read(fd1[0], buffer, BUFSIZE);
                    write(clnt_sock, buffer, BUFSIZE);

                    if (strcmp(buffer, "close the program.") == 0)
                        flag = false;

                    if (strcmp(buffer, "") == 0)
                        break;
                }
                if (flag == false)
                    break;
            }
        }
    }
    return 0;
}

void z_handler(int sig)
{
    pid_t pid;
    int status;

    pid = waitpid(-1, &status, WNOHANG);
    printf("removed proc id: %d \n", pid);
    printf("Returned data : %d \n\n", WEXITSTATUS(status));
}

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
