#include <netinet/in.h>
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/types.h> 

void processInput(char input[], char *args[]) {
  //Si los primeros dos carácteres son cd entonces tratamos al resto del buffer input como el segundo elemento de args.
  if (strncmp(input, "cd", 2) == 0 && (input[2] == ' ' || input[2] == '\0')) {
    args[0] = "cd";

    //Dividimos entonces la cadena en input después de los 3 primeros carácteres hasta el final de la cadena.
    args[1] = strtok(input + 3, "");
        
    args[2] = NULL;
  } else {
    // Si no es "cd", procesamos el input normalmente.
    int i = 0;
    char *arg = strtok(input, " ");
    while (arg != NULL) {
      args[i] = arg;
      i++;
      arg = strtok(NULL, " ");
    }
    args[i] = NULL;
  }
}

int main(int argc, char const* argv[]) 
{ 
  int servSockD = socket(AF_INET, SOCK_STREAM, 0); 
  if (servSockD < 0) {
    perror("Error creating socket");
    exit(1);
  }

  struct sockaddr_in servAddr; 

  servAddr.sin_family = AF_INET; 
  servAddr.sin_port = htons(8080); 
  servAddr.sin_addr.s_addr = INADDR_ANY; 

  if (bind(servSockD, (struct sockaddr*)&servAddr, sizeof(servAddr)) < 0) {
    perror("Error binding socket");
    exit(1);
  }

  if (listen(servSockD, 1) < 0) {
    perror("Error listening on socket");
    exit(1);
  }

  int clientSocket = accept(servSockD, NULL, NULL); 
  if (clientSocket < 0) {
    perror("Error accepting client connection");
    exit(1);
  }

  char input[256];
  char output[1024];

  while(1) {
    //Nos aseguramos de que los buffers estén "vacíos".
    memset(input, 0, sizeof(input));
    memset(output, 0, sizeof(output));

    //Se crea un pipe para redigir el output y cualquier error ocurrido al padre.
    int pipefd[2];
    if (pipe(pipefd) < 0) {
      perror("Error creating pipe");
      exit(1);
    }

    //Se crea un nuevo proceso para atender el comando a ejecutar.
    pid_t pid = fork();
    if (pid < 0) {
      perror("Error creating child process");
      exit(1);
    }

    //Proceso hijo.
    if (pid == 0) {
      //Se cierra la entrada de lectura del hijo.
      close(pipefd[0]);
      //Se redirige el output y cualquier error que genere el hijo hacia el pipe.
      dup2(pipefd[1], STDOUT_FILENO);
      dup2(pipefd[1], STDERR_FILENO);
      //Se cierra la entrada de escritura del hijo.
      close(pipefd[1]);

      //Se recibe la cadena.
      size_t bytesReceived = recv(clientSocket, input, sizeof(input), 0);

      //Se verifica que no hayan habido errores recibiendo el input.
      if (bytesReceived < 0) {
        perror("Error receiving data");
        exit(1);
      }

      //Se convierte el carácter siguiente al último del input a uno nulo. Esto es importante para funciones como strtok.
      input[bytesReceived] = '\0';

      //Si la cadena recibida es "salir" entonces se termina el proceso hijo.
			if (strcmp(input, "salir") == 0) {
        printf("Saliendo...\n");
				exit(0);
			}
      
      //Se crea el vector donde se guardarán los argumentos que usará execvp/chdir.
      char *args[10];

      for (int i = 0; i < 10; i++) { 
        args[i] = NULL; 
      }

      //Se ejecuta la función processInput que se encargará de dividir la cadena.
      processInput(input, args);

      //Si el comando principal es cd se utiliza chdir.
      if (strcmp(args[0], "cd") == 0){
        if (args[1] == NULL) {
          perror("No directory specified");
        } else {
          printf("Cambiando al directorio: %s\n", args[1]);

          if (chdir(args[1])) {
            perror("Chdir failed");
          }
        }
      } else {
        //Se ejecuta el comando mediante execvp.
        if(execvp(args[0], args)) {
          perror("Execvp failed");
        }
      }

      exit(0);
    } 
    //Proceso padre.
    else if (pid > 0) {
      size_t bytesRead;

      //Se cierra la entrada de escritura del padre.
      close(pipefd[1]);

      //Se espera a que el proceso hijo termine.
      wait(NULL);

      //Se lee del pipe y se pasan estos bytes a la variable output.
      bytesRead = read(pipefd[0], output, sizeof(output)-1); 

      //Si no se leyó nada del pipe (el comando no generó output), se envia el mensaje "Command Executed".
      if (bytesRead == 0) {
          snprintf(output, sizeof(output), "No output\n");
          bytesRead = strlen(output);
      } else {
          output[bytesRead] = '\0';
      }

      //Se manda el output al cliente.
      send(clientSocket, output, bytesRead, 0);

      //Si la cadena recibida es "salir" entonces se rompe el bucle.
			if (strcmp(input, "salir") == 0) {
        close(pipefd[0]);
				break;
			}

      //Se cierra la entrada de lectura del padre.
      close(pipefd[0]);
    } 
  }

  close(clientSocket);
  close(servSockD);
    
  return 0; 
}
