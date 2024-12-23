#include <netinet/in.h>
#include <stdio.h> 
#include <stdlib.h> 
#include <sys/socket.h>
#include <sys/types.h> 

def processInput(char Input[], char *args[]) {
  int i = 0;

  char *arg = strtok(input, " ");
  while (arg != NULL) {
    args[i] = arg;
    i++;
    arg = strtok(NULL, " ");
  }

  args[i] = NULL;
}

int main(int argc, char const* argv[]) 
{ 
  int servSockD = socket(AF_INET, SOCK_STREAM, 0); 
  if (servSockD < 0) {
    perror("Error creating socket");
    exit(1);
  }
  printf("Socket created successfully.\n");

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

  char input[255];

  while(1) {
    //Se crea un nuevo proceso para atender el comando a ejecutar.
    pid_t pid = fork();

    //Proceso hijo.
    if (pid == 0) {
      //Se recibe la cadena.
      size_t bytesReceived = recv(clientSocket, input, sizeof(input), 0);

      //Se verifica que el tamaño de esta sea válido.
      if (bytesReceived < 0) {
        perror("Error receiving data");
        exit(1);
      }
      //Si se envia un carácter vacío entonces termina el proceso hijo.
      else if (bytesReceived == 0) {
        print("Client disconnected.\n");
        exit(0);
      }

      //Se convierte el último carácter del input a uno nulo. Esto es importante para funciones como strtok.
      input[bytesReceived] = '\0';

      //Se crea el vector de argumentos que recibirá execvp. Se asume que ningún comando tendrá más de 9 argumentos sin contar el comando principal.
      char *args[10];

      //Se ejecuta la función processInput que se encargará de dividir la cadena.
      processInput(input, args);

      //Se ejecuta execvp.
      if(execvp(args[0], args) < 0) {
        perror("Exec failed");
      }

      //Termina el proceso hijo.
      exit(0);
    } 
    //Proceso padre.
    else if (pid > 0) {
      wait(NULL);

      //Si la cadena enviada tiene 0 caracteres, entonces se rompe el bucle.
      if (strlen(input) == 0) {
        break;
      }
    } 
    else {
      perror("Error creating child process");
    }
  }

  close(clientSocket);
  printf("Client disconnected.\n");
    
  return 0; 
}
