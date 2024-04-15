
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#define MAX_INPUT_LENGTH 1024

int main(int argc, char *argv[]) {
  char *server_ip = "127.0.0.1"; // Default IP
  int server_port = 5000;        // Default port

  if (argc > 1)
    server_ip = argv[1];
  if (argc > 2)
    server_port = atoi(argv[2]);

  struct sockaddr_in server_addr = {.sin_family = AF_INET,
                                    .sin_port = htons(server_port)};

  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
    perror("Socket creation failed");
    return -1;
  }

  if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
    perror("Invalid address/ Address not supported");
    close(sock);
    return -1;
  }

  if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    perror("Connection Failed");
    close(sock);
    return -1;
  }

  printf("Connected to %s:%d\n", server_ip, server_port);
  printf("use '/help' to list commands\n - USERNAME:");
  

  fd_set read_fds;
  struct timeval tv;
  int retval;

  while (1) {
    FD_ZERO(&read_fds);
    FD_SET(sock, &read_fds);
    FD_SET(STDIN_FILENO, &read_fds);

    tv.tv_sec = 2;
    tv.tv_usec = 0;

    retval = select(sock + 1, &read_fds, NULL, NULL, &tv);

    if (retval == -1) {
      perror("select()");
      break;
    } else if (retval) {
      if (FD_ISSET(sock, &read_fds)) {
        // Socket is ready to be read
        char recvbuf[MAX_INPUT_LENGTH];
        int len = recv(sock, recvbuf, sizeof(recvbuf) - 1, 0);
        if (len > 0) {
          recvbuf[len] = '\0';
          printf("-> %s\n", recvbuf);
        } else if (len == 0) {
          printf("Server closed the connection.\n");
          break;
        } else {
          perror("recv failed");
          break;
        }
      }
      if (FD_ISSET(STDIN_FILENO, &read_fds)) {
        // Standard input is ready to be read
        char sendbuf[MAX_INPUT_LENGTH];
        if (fgets(sendbuf, sizeof(sendbuf), stdin) == NULL) {
          perror("Error reading input");
          continue;
        }

        sendbuf[strcspn(sendbuf, "\n")] = 0;

        if (strcmp(sendbuf, "/exit") == 0) {
          printf("Exiting...\n");
          fflush(stdout);
          break;
        }

        if (send(sock, sendbuf, strlen(sendbuf), 0) < 0) {
          perror("Failed to send message");
          break;
        }
      }
    }
    // else {
    //   printf("No data within five seconds.\n");
    // }
    fflush(stdout);
  }

  close(sock);
  return 0;
}
