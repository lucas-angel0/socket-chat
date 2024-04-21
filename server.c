
#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define MAX_CLIENTS 100
#define MAX_USERNAME_LENGTH 100
#define MAX_INPUT_LENGTH 2048

typedef struct {
  int socket;
  char username[MAX_USERNAME_LENGTH];
} client_t;

client_t clients[MAX_CLIENTS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void send_to_client(int sock, const char *message) {
  send(sock, message, strlen(message), 0);
}

// Function to color a string randomly and return the new string
char *colorize_string(const char *inputString) {
  // ANSI color codes for foreground colors
  const char *colors[] = {
      "\033[31m", // Red
      "\033[32m", // Green
      "\033[33m", // Yellow
      "\033[34m", // Blue
      "\033[35m", // Magenta
      "\033[36m", // Cyan
      "\033[37m"  // White
  };

  // Initialize random number generator
  srand(time(NULL));

  // Get a random index for colors array
  int randomIndex = rand() % (sizeof(colors) / sizeof(colors[0]));

  // Calculate needed buffer size
  size_t neededSize = strlen(inputString) + strlen(colors[randomIndex]) +
                      5; // +5 for escape sequence and null terminator

  // Allocate memory for the new string
  char *coloredString = malloc(neededSize);

  if (coloredString == NULL) {
    fprintf(stderr, "Memory allocation failed\n");
    exit(1);
  }

  // Construct the new string
  strcpy(coloredString, colors[randomIndex]);
  strcat(coloredString, inputString);
  strcat(coloredString, "\033[0m"); // Reset color

  return coloredString;
}

const char *find_username_by_socket(int sock) {
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (clients[i].socket == sock) {
      return clients[i].username;
    }
  }
  return NULL;
}

void broadcast_message(const char *message, int sender_sock) {
  const char *username = find_username_by_socket(sender_sock);
  char formatted_message[MAX_INPUT_LENGTH + MAX_USERNAME_LENGTH + 3];

  if (username) {

    snprintf(formatted_message, sizeof(formatted_message), "%s: %s", username,
             message);
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
      if (clients[i].socket != 0 && clients[i].socket != sender_sock) {
        send_to_client(clients[i].socket, formatted_message);
      }
    }
    pthread_mutex_unlock(&clients_mutex);
  }
}

void list_users(int sock) {
  char buffer[MAX_INPUT_LENGTH] = "Connected users:\n";
  pthread_mutex_lock(&clients_mutex);
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (clients[i].socket != 0) {
      strncat(buffer, clients[i].username, sizeof(buffer) - strlen(buffer) - 1);
      strncat(buffer, "\n", sizeof(buffer) - strlen(buffer) - 1);
    }
  }
  pthread_mutex_unlock(&clients_mutex);
  send_to_client(sock, buffer);
}

void whisper_message(const char *message, const char *username,
                     int sender_sock) {
  const char *sender_username = find_username_by_socket(sender_sock);
  char formatted_message[MAX_INPUT_LENGTH + MAX_USERNAME_LENGTH +
                         3]; // Enough space for the message and the username

  pthread_mutex_lock(&clients_mutex);
  if (sender_username) {
    // Format the message to indicate it's a private whisper
    snprintf(formatted_message, sizeof(formatted_message), "%s (whisper): %s",
             sender_username, message);
    // Search for the recipient by username and send the message if found
    for (int i = 0; i < MAX_CLIENTS; i++) {
      if (clients[i].socket != 0 &&
          strcmp(clients[i].username, username) == 0) {
        send_to_client(clients[i].socket, formatted_message);
        break;
      }
    }
  }
  pthread_mutex_unlock(&clients_mutex);
}
void handle_command(const char *buffer, int sock) {
  if (strncmp(buffer, "/listusers", 10) == 0) {
    list_users(sock);
  } else if (strncmp(buffer, "/msg ", 5) == 0) {
    char *space = strchr(buffer + 5, ' ');
    if (space != NULL) {
      *space = '\0'; // Split the username and the message
      whisper_message(space + 1, buffer + 5, sock);
    }
  } else if (strncmp(buffer, "/help", 5) == 0) {
    const char help_message[] =
        "Available commands:\n"
        "/listusers - Lists all connected users\n"
        "/msg <username> <message> - Send a private message to <username>\n"
        "/help - Shows this help message\n";
    send_to_client(sock, help_message);
  } else {
    char err_msg[] = "Unknown command. Type /help for a list of commands.\n";
    send_to_client(sock, err_msg);
  }
}

void add_client(int client_sock, const char *username) {
  pthread_mutex_lock(&clients_mutex);
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (clients[i].socket == 0) {
      clients[i].socket = client_sock;
      strncpy(clients[i].username, colorize_string(username),
              MAX_USERNAME_LENGTH);
      clients[i].username[MAX_USERNAME_LENGTH - 1] =
          '\0'; // Ensure null termination
      break;
    }
  }
  pthread_mutex_unlock(&clients_mutex);
}

void remove_client(int client_sock) {
  pthread_mutex_lock(&clients_mutex);
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (clients[i].socket == client_sock) {
      clients[i].socket = 0;
      clients[i].username[0] = '\0';
      break;
    }
  }
  pthread_mutex_unlock(&clients_mutex);
}

void *handle_client(void *client_sock) {
  int sock = *((int *)client_sock);
  free(client_sock);

  char username[MAX_USERNAME_LENGTH];
  char buffer[MAX_INPUT_LENGTH];
  int bytes_read = recv(sock, username, MAX_USERNAME_LENGTH - 1, 0);
  if (bytes_read <= 0) {
    printf("Failed to read username from client.\n");
    close(sock);
    return NULL;
  }
  username[bytes_read] = '\0'; // Null-terminate the username

  add_client(sock, username);
  printf("%s joined the chat\n", username);

  while ((bytes_read = recv(sock, buffer, MAX_INPUT_LENGTH - 1, 0)) > 0) {
    buffer[bytes_read] = '\0';
    if (buffer[0] == '/') {
      handle_command(buffer, sock);
    } else {
      broadcast_message(buffer, sock);
    }
  }

  if (bytes_read == 0) {
    printf("%s disconnected.\n", username);
  } else {
    perror("recv failed");
  }

  char goodbye_msg[MAX_INPUT_LENGTH];
  snprintf(goodbye_msg, sizeof(goodbye_msg), "%s has left the chat.\n",
           username);
  broadcast_message(goodbye_msg, sock);
  remove_client(sock);
  close(sock);
  return NULL;
}

int main(int argc, char *argv[]) {
  int port = 5000;

  if (argc > 1)
    port = atoi(argv[1]);

  struct sockaddr_in saddr = {.sin_family = AF_INET,
                              .sin_addr.s_addr = htonl(INADDR_ANY),
                              .sin_port = htons(port)};

  int server = socket(AF_INET, SOCK_STREAM, 0);
  if (server < 0) {
    perror("Socket creation failed");
    return -1;
  }

  if (bind(server, (struct sockaddr *)&saddr, sizeof saddr) < 0) {
    perror("Bind failed");
    close(server);
    return -1;
  }

  if (listen(server, 5) < 0) {
    perror("Listen failed");
    close(server);
    return -1;
  }

  printf("Server is listening on port %d\n", port);
  memset(clients, 0, sizeof(clients));

  while (1) {
    struct sockaddr_in caddr;
    socklen_t csize = sizeof caddr;
    int *client_sock = malloc(sizeof(int));
    *client_sock = accept(server, (struct sockaddr *)&caddr, &csize);
    if (*client_sock < 0) {
      perror("Accept failed");
      free(client_sock);
      continue;
    }

    pthread_t tid;
    if (pthread_create(&tid, NULL, handle_client, client_sock) != 0) {
      perror("Failed to create thread");
      close(*client_sock);
      free(client_sock);
    }
    pthread_detach(tid);
  }

  close(server);
  return 0;
}
