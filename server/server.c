#include <netinet/in.h>
#include <stdio.h> 
#include <stdlib.h> 
#include <sys/socket.h>
#include <sys/types.h> 

int main(int argc, char const* argv[]) 
{ 
    int servSockD = socket(AF_INET, SOCK_STREAM, 0); 
    if (servSockD < 0) {
      perror("Error creating socket");
      exit(1);
    }
    printf("Socket created successfully.\n");

    char serMsg[255] = "Message from the server to the client 'Hello Client'"; 

    struct sockaddr_in servAddr; 

    servAddr.sin_family = AF_INET; 
    servAddr.sin_port = htons(8080); 
    servAddr.sin_addr.s_addr = INADDR_ANY; 

    if (bind(servSockD, (struct sockaddr*)&servAddr, sizeof(servAddr)) < 0) {
      perror("Error binding socket");
      exit(1);
    }
    printf("Socket bound successfully.\n");

    if (listen(servSockD, 1) < 0) {
      perror("Error listening on socket");
      exit(1);
    }
    printf("Server is listening for connections...\n");

    int clientSocket = accept(servSockD, NULL, NULL); 
    if (clientSocket < 0) {
      perror("Error accepting client connection");
      exit(1);
    }
    printf("Client connected successfully.\n");

    send(clientSocket, serMsg, sizeof(serMsg), 0); 

    return 0; 
}
