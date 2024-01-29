#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUFSIZE 100
#define AlPHA_SIZE 24

void error_handling(char *message);

int main(int argc, char *argv[])
{
    char aleady_input[AlPHA_SIZE] = {""}; // variable untuk menampilkan data yang dimasukkan
    int alin_cnt = 0;                     // menyimpan indeks dari string yang sudah dimasukkan
    char str[BUFSIZE];                    // Variabel string untuk menerima jawaban yang benar yang dimasukkan oleh pengguna
    char serv_message[BUFSIZE];           // variable untuk menerima string yang dikirim dari server

    int sock;
    char message[BUFSIZE]; // variable untuk menerima pesan dari server
    int str_len = 0;

    int a, b, k;

    bool flag = true;

    struct sockaddr_in serv_addr;

    if (argc != 3)
    {
        printf("Usage : %s <IP> <port>\n", argv[0]);
        exit(1);
    }

    sock = socket(PF_INET, SOCK_STREAM, 0);

    if (sock == -1)
        error_handling("socket() error");

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
        error_handling("connect() error!");

    read(sock, &str_len, sizeof(int));
    printf("correct answer character length : %d\n", str_len);
    str_len = 0;

    while (true)
    {
        // Berikan panduan untuk karakter yang sudah dimasukkan
        if (alin_cnt != 0)
            printf("Characters already entered : [ ");
        for (int a = 0; a < alin_cnt; a++)
            printf("%c ", aleady_input[a]);
        if (alin_cnt != 0)
            printf("]\n");

       // mendapatkan jawaban yang benar
        printf("Enter the alphabet or whole word: ");
        scanf("%s", str);

        bool check_alpha = true;
        // Ubah string input menjadi huruf kecil dan periksa apakah itu alfabet!
        for (k = 0; k < strlen(str); k++)
        {

            if (!isalpha(str[k])) // periksa apakah itu abjad
            {
                check_alpha = false;
                break;
            }
            str[k] = tolower(str[k]); // ubah semua menjadi huruf kecil 
        }

        // Lewati jika karakter non-abjad dicampur dalam karakter atau string yang dimasukkan
        if (!check_alpha)
        {
            printf("Please enter only the alphabet!\n");
            continue;
        }
        // Lewati jika string sudah dimasukkan!
        if (strlen(str) == 1)
        {
            bool check_aleady = true;
            for (int b = 0; b < alin_cnt; b++)
            {
                if (aleady_input[b] == str[0])
                {
                    printf("This character has already been entered!\n");
                    check_aleady = false;
                    break;
                }
            }
            if (!check_aleady)
                continue;
        }

        // Simpan kata-kata yang dimasukkan dalalm pedoman
        if (strlen(str) == 1)
        {
            aleady_input[alin_cnt] = str[0];
            alin_cnt++;
        }

        write(sock, str, BUFSIZE);

        while (true)
        {
            read(sock, serv_message, BUFSIZE);
            if (strcmp(serv_message, "") == 0)
                break;
            printf("%s\n", serv_message);

            if (strcmp(serv_message, "close the program.") == 0)
                flag = false;
        }

        if (flag == false)
            break;
            
    }

    close(sock);
    return 0;
}

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
