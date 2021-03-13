//Index-Number : 18020569
//Run File : gcc -pthread 18020569_server.c -o 18020569_server.c
//Compile : ./18020569_server 8080 /

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

const char* const HTTP_OK_HEAD = "HTTP/1.1 200 OK\n"; 

const char* const HTTP_404 = "HTTP/1.1 404 Not Found\nContent-Length: 146\nConnection: close\nContent-Type: text/html\n\n<html><head>\n<title>404 Error</title>\n</head>\n<body>\n<h1>File Not Found</h1>\n<p>The requested resource was not found</p>\n</body>\n</html>\n";

const char* const HTTP_403 = "HTTP/1.1 403 Forbidden\nContent-Length: 166\nConnection: close\nContent-Type: text/html\n\n<html><head>\n<title>403 Error</title>\n</head><body>\n<h1>Error: </h1>\n<p>The requested URL or File Type is not allowed.</p>\n</body></html>\n";

const char* const CONT_TYPE_BASE = "Content-Type: ";
const char* const CONT_TYPE_HTML = "Content-Type: text/html\n\n";
const char* const CONTENT_LEN_BASE = "Content-Length: ";
const char* const SERVER = "Reipache 1.0\n";
const char* const CONN_CLOSE = "Connection: close\n";

const char* const INDEX_PAGE_BODY = "<html><head><title>Welcome</title></head><body style=\"background-color:#10a5d6; color:#b3ecff; padding:30px; text-align: center;\"><div><p style=\"text-align: left;color: rgb(253, 253, 253);\">H.S.H.Perera | 18020569</p></div><div style=\"background-color:#066b8d; color:#b3ecff; padding:30px;\"><h1>WEB SERVER 18020569</h1><h3>Computer Networks - IS2111 <br><span style=\"color:rgb(255, 255, 255);font-weight:bold\">Assignment</span></h3></div><div style=\"background-color:#1c8fb6; color:#b3ecff; padding:20px; text-align: center;\"><h1 style=\"font-size: 70px;\"><b>Welcome to my Web Server</b></h1></div></body></html>";

//Size constants

#define BUF_SIZE 2048

//Content Types
struct {
   char *ext;
   char *contentType;
} extensions [] = {
   {"gif", "image/gif" },
   {"jpg", "image/jpg" },
   {"jpeg","image/jpeg"},
   {"png", "image/png" },
   {"ico", "image/ico" },
   {"htm", "text/html" },
   {"html","text/html" },
   {0,0} };

const char* get_file_ext(const char* filename) {
    const char *dot = strrchr(filename, '.');
    if (!dot || dot == filename) return "";
    return dot + 1;
}

int getLength(FILE *fp) {
   int size = 0;
   fseek(fp, 0, SEEK_END);
   size = ftell(fp);
   rewind(fp);
   return size;
}


void* processRequest(void* newSock) {

   printf("processRequest:\n");
   int x =  *((int *)newSock);
   char buffer[BUF_SIZE];
   char *method;
   char *path;

   //Get the request body from the socket
   if (recv(x, buffer, BUF_SIZE-1, 0) == -1) {
      perror("Incoming request error handling \n");
      close(x);
      return NULL;
   }
 
   if (buffer[0] == '\0') {
      close(x);
      return NULL;
   }

   //Print request to console
   printf("%s\n", buffer);
   
   method = strtok(buffer, " ");
   path = strtok(NULL, " ");
 
   //Send back 403 if the request type doesn't match
   if (strcasecmp(method, "GET") != 0) {
      write(x, HTTP_403, strlen(HTTP_403));
      close(x);
      return(NULL);
   }

   //Get file extension
   const char* ext;
   ext = get_file_ext(path);

   printf("Requested resource path: %s\n", path);
   printf("File extension: %s\n", ext);

   //Not accepting relative paths to parent directories 
   if (path[1] == '.') {
      write(x, HTTP_403, strlen(HTTP_403));
      close(x);
      return(NULL);
   }



   //Serve the default index page and return
   if (strcmp(path, "/") == 0) {
      write(x, HTTP_OK_HEAD, strlen(HTTP_OK_HEAD));
      write(x, SERVER, strlen(SERVER));
      char defaultContentLength[40];
      sprintf(defaultContentLength, "%s %lu\n", CONTENT_LEN_BASE, strlen(INDEX_PAGE_BODY));
      write(x, defaultContentLength, strlen(defaultContentLength));
      write(x, CONN_CLOSE, strlen(CONN_CLOSE));
      write(x, CONT_TYPE_HTML, strlen(CONT_TYPE_HTML));
      write(x, INDEX_PAGE_BODY, strlen(INDEX_PAGE_BODY));
      close(x);
      return NULL;
   }

   //Determine content type from file extenion
   int i;
   char *ctype = NULL;
   for (i=0; extensions[i].ext != 0; i++) {
      if (strcmp(ext, extensions[i].ext) == 0) {
         ctype = extensions[i].contentType;
         break;
      }
   }

   //When a filename is provided without an extension or the content type isn't supported.
   if (ctype == NULL) {
      write(x, HTTP_403, strlen(HTTP_403));
      close(x);
      return(NULL);
   }

   //Remove leading slash from path
   path++;

   //Try to open the requested file
   FILE *fp;
   fp = fopen(path, "rb");
   if (fp == NULL) {
      write(x, HTTP_404, strlen(HTTP_404));
      close(x);
      return(NULL);
   } else {
      printf("File opened\n");
   }

   //Requested resource was found and opened
   int contentLength;
   contentLength = getLength(fp);
   if (contentLength < 0) {
      printf("Length is requested file < 0. Returning 403.\n");
      write(x, HTTP_403, strlen(HTTP_403));
      close(x);
      return(NULL);
   }

   //Write back headers and content
   write(x, HTTP_OK_HEAD, strlen(HTTP_OK_HEAD));
   write(x, SERVER, strlen(SERVER));

   //Format content length
   char contentLengthString[40];
   sprintf(contentLengthString, "%s %d\n", CONTENT_LEN_BASE, contentLength);
   write(x, contentLengthString, strlen(contentLengthString));

   write(x, CONN_CLOSE, strlen(CONN_CLOSE));

   //Format the content type
   char contentType[80];
   sprintf(contentType, "%s %s\n\n", CONT_TYPE_BASE, ctype);
   write(x, contentType, strlen(contentType));

   //Write each byte of the file to the socket
   int current_char = 0;
   do {
      current_char = fgetc(fp);
      write(x, &current_char, sizeof(char));
   } while (current_char != EOF);

   close(x);
   return NULL;
}

int main(int argc, char *argv[]) {

   if (argc < 3) {
      printf("Enter: <./servername> <port_number> <file_path> \n");
      printf("Enter '/' if the server file is in the current directory \n");
      exit(1);
   }

   int sock, new_socket, status;
   socklen_t addrlen;
   struct sockaddr_in address;
   unsigned short port = (unsigned short) strtoul(argv[1], NULL, 0);

   //Security checks
   if (strcmp(argv[2], "/") != 0) {
      if (argv[2][0] == '.') {
         perror("Error: Enter path relativly to the server\n");
         exit(1);
      }

      if (chdir(argv[2]) < 0) { 
         printf("Error: Directory Cannot be Opened %s\n", argv[2]);
         printf("NOTE: Only paths relative to this program's directory are allowed.\n");
         exit(1);
      }
   }

   //Create new socket
   if ((sock = socket(AF_INET, SOCK_STREAM, 0)) > 0) {
      printf("Socket Created\n");
   }

   //Configure address attributes
   address.sin_family = AF_INET;
   address.sin_addr.s_addr = INADDR_ANY;
   address.sin_port = htons(port);
    
   //Bind sockets
   if (bind(sock, (struct sockaddr *) &address, sizeof(address)) == 0) {
      printf("Binding Complete\n");
   } else {
      perror("Binding socket error");
      exit(1);
   }

   pthread_t thread;
   while (1) {
      //Enable connection requests
      if (listen(sock, 10) < 0) {
         perror("Server_Listen");
         exit(1);
      }

      if ((new_socket = accept(sock, (struct sockaddr *) &address, &addrlen)) < 0) {    
         perror("Server Connection cannot be accepted");
         continue;
      }
      else if (new_socket > 0) {
         printf("Client connection is Active\n");
      }

      //Requests hadeling treads
      if (pthread_create(&thread, NULL, processRequest, &new_socket)) {
         perror("Error creating thread");
         continue;
      }

      pthread_detach(thread);
   }

   close(sock);
   return 0;
}

