#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <winsock2.h>
#include <windows.h>

#define LENGTH 2048
#define INET_ADDRSTRLEN 16

// Global variables
volatile sig_atomic_t flag = 0;
SOCKET sockfd = 0;
char name[32];

void str_overwrite_stdout() {
    printf("%s", "> ");
    fflush(stdout);
}

void str_trim_lf(char* arr, int length) {
    int i;
    for (i = 0; i < length; i++) { // trim \n
        if (arr[i] == '\n') {
            arr[i] = '\0';
            break;
        }
    }
}

void catch_ctrl_c_and_exit(int sig) {
    flag = 1;
}

void send_msg_handler(LPVOID lpParam) {
    char message[LENGTH] = {};
    char buffer[LENGTH + 32] = {};

    while (1) {
        str_overwrite_stdout();
        fgets(message, LENGTH, stdin);
        str_trim_lf(message, LENGTH);

        if (strcmp(message, "exit") == 0) {
            break;
        }
        else {
            sprintf(buffer, "%s: %s\n", name, message);
            send(sockfd, buffer, strlen(buffer), 0);
        }

        memset(message, 0, sizeof(message));
        memset(buffer, 0, sizeof(buffer));
    }

    catch_ctrl_c_and_exit(2);
}

void recv_msg_handler(LPVOID lpParam) {
    char message[LENGTH] = {};
    while (1) {
        int receive = recv(sockfd, message, LENGTH, 0);
        if (receive > 0) {
            printf("%s", message);
            str_overwrite_stdout();
        }
        else if (receive == 0) {
            break;
        }
        else {
            // Handle error
        }
        memset(message, 0, sizeof(message));
    }
}

int main(int argc, char** argv) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        fprintf(stderr, "WSAStartup failed.\n");
        return EXIT_FAILURE;
    }

    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        return EXIT_FAILURE;
    }
    char tmpip[INET_ADDRSTRLEN];
    printf("Enter Server IP:\n");
    if (fgets(tmpip, sizeof(tmpip), stdin) == NULL) {
        perror("Error Reading input!!");
        return EXIT_FAILURE;
    }
    char* ip = tmpip;
    int port = atoi(argv[1]);

    signal(SIGINT, catch_ctrl_c_and_exit);

    printf("Please enter your name: ");
    fgets(name, 32, stdin);
    str_trim_lf(name, strlen(name));


    if (strlen(name) > 32 || strlen(name) < 2) {
        printf("Name must be less than 30 and more than 2 characters.\n");
        return EXIT_FAILURE;
    }

    struct sockaddr_in server_addr;

    /* Socket settings */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == INVALID_SOCKET) {
        fprintf(stderr, "socket() failed: %ld\n", WSAGetLastError());
        WSACleanup();
        return EXIT_FAILURE;
    }
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip);
    server_addr.sin_port = htons(port);


    // Connect to Server
    int err = connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (err == SOCKET_ERROR) {
        fprintf(stderr, "connect() failed: %ld\n", WSAGetLastError());
        closesocket(sockfd);
        WSACleanup();
        return EXIT_FAILURE;
    }

    // Send name
    send(sockfd, name, 32, 0);

    printf("=== WELCOME TO THE CHATROOM ===\n");

    HANDLE send_msg_thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)send_msg_handler, NULL, 0, NULL);
    if (send_msg_thread == NULL) {
        fprintf(stderr, "CreateThread() failed: %ld\n", GetLastError());
        return EXIT_FAILURE;
    }

    HANDLE recv_msg_thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)recv_msg_handler, NULL, 0, NULL);
    if (recv_msg_thread == NULL) {
        fprintf(stderr, "CreateThread() failed: %ld\n", GetLastError());
        return EXIT_FAILURE;
    }

    while (1) {
        if (flag) {
            printf("\nBye\n");
            break;
        }
    }

    closesocket(sockfd);
    WSACleanup();

    return EXIT_SUCCESS;
}

