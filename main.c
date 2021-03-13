#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>


void doWrite(int new_socket, char* headerText, int headerLen, char* filename){

    FILE *fp = fopen(filename, "r");
    //Handle File Not Found Error
    if (!fp) {
      fp = fopen("404.html", "r");
    }
    
    char *contents;
    int fileSize = 0;

    //get file size
    fseek(fp, 0L, SEEK_END);
    fileSize = ftell(fp);
    fseek(fp, 0L, SEEK_SET);
    contents = malloc(fileSize+1);

    //Read the file 
    size_t size=fread(contents,1,fileSize,fp);
    contents[size]=0;

    //Close file stream
    fclose(fp);
    free(contents);
    
    //Write Headers
    write(new_socket, "HTTP/1.1 200 OK\n", 16);
    write(new_socket, headerText, headerLen );
    send(new_socket, contents, fileSize, 0);
    
    //Close socket
    close(new_socket);
}

void doRoute(int new_socket, char* request){

  //Handle root
  if(strncmp(request, "/", 10) == 0){
    doWrite(new_socket,"Content-Type: text/html\r\n\r\n", 28, "index.html");
  }else{
    //Remove the precefing "/" to get the file name
    memmove (request, request+1, strlen(request+1) + 1); 
    if (strstr(request, ".html") != NULL) {
      doWrite(new_socket, "Content-Type: text/html\r\n\r\n", 28, request);
    } else if (strstr(request, ".png") != NULL || strstr(request, ".jpg") != NULL || strstr(request, ".jpeg") != NULL) {
      doWrite(new_socket, "Content-Type: image/jpeg\r\n\r\n",28, request);
    } else if (strstr(request, ".pdf") != NULL) {
      doWrite(new_socket, "Content-Type: application/pdf\r\n\r\n",28, request);
    } else{
      doWrite(new_socket, "Content-Type: text/html\r\n\r\n", 28, "notsupported.html");
    }
  }
}

int main()
{
  int create_socket, new_socket;
  socklen_t addrlen;
  int bufsize = 1024;
  char *buffer = malloc(bufsize);
  struct sockaddr_in address;

  if ((create_socket = socket(AF_INET, SOCK_STREAM, 0)) > 0)
  {
    printf("Socket created\n");
  }

  address.sin_family = AF_INET; //set address family to ipv4
  address.sin_addr.s_addr = INADDR_ANY; 
  address.sin_port = htons(7000); //host byte to network byte

  if (bind(create_socket, (struct sockaddr *)&address, sizeof(address)) == 0)
  {
    printf("Server started on port 7000\n");
  }

  while (1)
  {
    if (listen(create_socket, 10) < 0){
      perror("server: listen");
      exit(1);
    }

    if ((new_socket = accept(create_socket, (struct sockaddr *)&address, &addrlen)) < 0){
      perror("server: accept");
      exit(1);
    }

    if (new_socket > 0){
      printf("Incoming request identified\n");
    }

    recv(new_socket, buffer, bufsize, 0);

    //Extract the file path from incoming request
    strtok(buffer, " ");
    char* route  = strtok(NULL, " ");
    printf("%s\n", route);
    doRoute(new_socket, route);
  }
  close(create_socket);
  return 0;
}

