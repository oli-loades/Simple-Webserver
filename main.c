#include < ctype.h > 
#include < sys / types.h > 
#include < sys / stat.h > 
#include < sys / socket.h >
#include < netinet / in .h >
#include < stdlib.h > 
#include < stdio.h > 
#include < string.h > 
#include < unistd.h >

#define SIZE sizeof(struct sockaddr_in)

int isGet(char request[2048]);

void sendHeader(int client, int type);

void respond(char * dir, char * fileName, int client);

int setUp(char * dir, int * portNum, int * processes);

int notRunning(int pid);

void sendErrorFile(char * dir, int client);

int getLen(char request[2048]);

int main(int argc, char * argv[]) {

  int portNum, processes, pid, len;
  pid = getpid();
  if (notRunning(pid) == 0) {
    char * dir = malloc(100);

    if (setUp(dir, & portNum, & processes) == 0) {

      int sockfd, clientsockfd, isValidRequest, size;
      char request[2048];
      char requestCopy[2048];
      struct sockaddr_in server;
      sockfd = socket(PF_INET, SOCK_STREAM, 0);
      server.sin_family = AF_INET;
      server.sin_port = htons(portNum); //portNum 
      server.sin_addr.s_addr = INADDR_ANY;
      bind(sockfd, (struct sockaddr * ) & server, SIZE);
      listen(sockfd, 5);

      while (1) {

        clientsockfd = accept(sockfd, NULL, NULL);

        if (fork() == 0) {

          while (recv(clientsockfd, & request, 2048, 0) > 0) {

            len = getLen(request);
            request[len - 1] = '\0';
            char * requestCopy = malloc(len);
            strncpy(requestCopy, request, len);
            int i = 0;
            char * requestTokens[100];
            char * token = strtok(requestCopy, " \t");

            while (token != NULL) {

              requestTokens[i] = malloc(strlen(token) + 1);
              strncpy(requestTokens[i], token, strlen(token));
              token = strtok(NULL, " ");
              i++;

            }

            free(requestCopy);
            size = i;
            int isValidRequest = isGet(requestTokens[0]);

            if (isValidRequest == 0 && i >= 2) {

              len = strlen(dir);
              len += strlen(requestTokens[1]);
              char * fileName = malloc(len + 1);
              strncpy(fileName, dir, strlen(dir));
              strncat(fileName, requestTokens[1], strlen(requestTokens[1]));
              respond(dir, fileName, clientsockfd);
              free(fileName);

            } else {

              send(clientsockfd, "Invalid request\n", 16, 0);

            }

            for (i = 0; i < size; i++) {

              free(requestTokens[i]);

            }

          }

          close(clientsockfd);

          exit(0);

        }
        close(clientsockfd);
        free(dir);

      }

    } else {

      printf("config file not found\n");

    }

  } else {

    printf("running\n");
  }

  return 0;
}

int notRunning(int pid) {
    
  int notRunning = 1; //it is running 
  FILE * file = fopen("/home/14054494/var/run/web.pid", "r");

  if (file == NULL) {

    if (file = fopen("/home/14054494/var/run/web.pid", "w")) {

      fprintf(file, "%d", pid);
      fclose(file);
      notRunning = 0;

    } else {

      perror("error");

    }

  }

  return notRunning;

}

int setUp(char * dir, int * portNum, int * processes) {

  int success = 1;
  char tempDir[30] = "/home/14054494/etc/httpd.conf";
  FILE * config;

  if (config = fopen(tempDir, "r")) {

    fscanf(config, "%s %d %d", tempDir, portNum, processes);
    fclose(config);
    strncpy(dir, tempDir, strlen(tempDir));
    dir += 3; //ignores BOM 
    success = 0;

  }

  return success;

}

int isGet(char request[2048]) {
    
  return strncmp(request, "GET", 3);

}

int getLen(char request[2048]) {

  int maxLen = strlen(request);
  int i = 0;

  while ((request[i] != '\n') || (i == maxLen)) {

    i++;

  }

  return i;

}

void respond(char * dir, char * fileName, int client) {

  struct stat stat_buf;
  int valid = 1;
  FILE * file;

  if (file = fopen(fileName, "r")) {

    stat(fileName, & stat_buf);
    int len = strlen(fileName);
    char * type = & fileName[len - 4];

    if (strcmp(type, "html") == 0) {

      sendHeader(client, 0);
      sendfile(client, file, NULL, stat_buf.st_size);
      valid = 0;

    } else if (strcmp(type, ".jpg") == 0) {

      sendHeader(client, 1);
      sendfile(client, file, NULL, stat_buf.st_size);
      valid = 0;

    } else if (strcmp(type, ".gif") == 0) {

      sendHeader(client, 2);
      sendfile(client, file, NULL, stat_buf.st_size);
      valid = 0;

    } else {

      send(client, "File type not supported\n", 24, 0);

    }

    if (valid == 1) {

      sendErrorFile(dir, client);

    } else {

      send(client, "file sent\n", 10, 0);

    }

    fclose(file);

  } else {

    sendErrorFile(dir, client);
  }

}

void sendErrorFile(char * dir, int client) {

  sendHeader(client, -1);
  char * temp = malloc(strlen(dir) + 9);
  strncpy(temp, dir, strlen(dir));
  strncat(temp, "404.html\0", 9);
  struct stat stat_buf;
  FILE * errorFile;

  if (errorFile = fopen(temp, "r")) {

    stat(temp, & stat_buf);
    sendfile(client, errorFile, NULL, stat_buf.st_size);
    send(client, "file sent\n", 10, 0);
    fclose(errorFile);

  } else {

    send(client, "Error 404 - File not found\n", 27, 0);

  }

  free(temp);

}

void sendHeader(int client, int type) {

  if (type == -1) {

    send(client, "HTTP/1.1 404 Not Found\n", 23, 0);

  } else {

    send(client, "HTTP/1.1 200 OK\n", 16, 0);

  }

  send(client, "Server: U08225Server14054494\n", 30, 0);

  switch (type) {
  case -1:
  case 0:
    send(client, "Content-type: text/html; charset=UTF-8\n", 40, 0);
    break;
  case 1:
    send(client, "Content-type: image/jpeg\n", 25, 0);
    break;
  case 2:
    send(client, "Content-type: image/gif\n", 24, 0);
    break;
  default:
    send(client, "ERROR\n", 8, 0);
  }

  send(client, "Connection: close\n", 18, 0);
  send(client, "\n", 2, 0);

}
