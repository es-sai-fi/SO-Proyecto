#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <fcntl.h>

void processInput(char input[], char *args[])
{
  // Si los primeros dos carácteres son cd entonces tratamos al resto del buffer input como el segundo elemento de args.
  if (strncmp(input, "cd", 2) == 0 && (input[2] == ' ' || input[2] == '\0'))
  {
    args[0] = "cd";

    // Dividimos entonces la cadena en input después de los 3 primeros carácteres hasta el final de la cadena.
    args[1] = strtok(input + 3, "");

    args[2] = NULL;
  }
  else
  {
    // Si no es "cd", procesamos el input normalmente.
    int i = 0;
    char *arg = strtok(input, " ");
    while (arg != NULL)
    {
      args[i] = arg;
      i++;
      arg = strtok(NULL, " ");
    }
    args[i] = NULL;
  }
}

int main(int argc, char const *argv[])
{
  int servSockD = socket(AF_INET, SOCK_STREAM, 0);
  if (servSockD < 0)
  {
    perror("Error creating socket");
    exit(1);
  }

  struct sockaddr_in servAddr;

  servAddr.sin_family = AF_INET;
  servAddr.sin_port = htons(8080);
  servAddr.sin_addr.s_addr = INADDR_ANY;

  if (bind(servSockD, (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0)
  {
    perror("Error binding socket");
    exit(1);
  }

  if (listen(servSockD, 1) < 0)
  {
    perror("Error listening on socket");
    exit(1);
  }

  int clientSocket = accept(servSockD, NULL, NULL);
  if (clientSocket < 0)
  {
    perror("Error accepting client connection");
    exit(1);
  }

  char input[256];
  char output[1024];
  // Se crea el pipe para redigirir los streams a este.
  int pipefd[2];
  pipe2(pipefd, O_NONBLOCK);
  int keepRunning;

  // Se crea un nuevo proceso para atender el comando a ejecutar.
  pid_t pid = fork();

  if (pid == 0) // Hijo original.
  {
    size_t bytesRead, bytesReceived;
    int keepRunning = 1;
    // Se crea el vector donde se guardarán los argumentos que usará execvp/chdir.
    char *args[10];

    // Se redirijen los streams STDOUT y STDERR al pipe (streams de hijo original).
    dup2(pipefd[1], STDOUT_FILENO);
    dup2(pipefd[1], STDERR_FILENO);
    // Se cierra el descriptor de escritura del pipe en el proceso hijo original.
    close(pipefd[1]);

    while (keepRunning)
    {
      // Nos aseguramos de que los buffers estén "vacíos".
      memset(input, 0, sizeof(input));
      memset(output, 0, sizeof(output));

      // Nos aseguramos que el vector args sea limpiado en cada iteración.
      for (int i = 0; i < 10; i++)
      {
        args[i] = NULL;
      }

      // Se recibe la cadena.
      bytesReceived = recv(clientSocket, input, sizeof(input), 0);

      // Se convierte el carácter siguiente al último del input a uno nulo. Esto es importante para funciones como strtok.
      input[bytesReceived] = '\0';

      // Se ejecuta la función processInput que se encargará de dividir la cadena.
      processInput(input, args);

      // Si la cadena recibida es "salir" entonces se cambia el valor de "continue" a false.
      if (strcmp(input, "salir") == 0)
      {
        snprintf(output, sizeof(output), "Saliendo...\n");
        keepRunning = 0;
      }
      // Si el comando principal es cd se utiliza chdir.
      else if (strcmp(args[0], "cd") == 0)
      {
        if (args[1] == NULL)
        {
          snprintf(output, sizeof(output), "No directory specified\n");
        }
        else
        {
          if (chdir(args[1]) != 0)
          {
            snprintf(output, sizeof(output), "Chdir failed\n");
          }
        }
      }
      else
      {
        pid_t executerChild = fork();
        if (executerChild == 0) // Nuevo hijo, se encarga de ejecutar comandos con execvp (su padre es el hijo original).
        {
          // Se cierran los descriptures del pipe en el proceso hijo ejecutor.
          close(pipefd[0]);
          close(pipefd[1]);

          // El nuevo hijo ejecuta el comando
          if (execvp(args[0], args) == -1)
          {
            perror("Execvp failed");
            exit(1);
          }
        }
        else if (pid > 0)
        {
          // Es importante que las siguientes instrucciones no se ejecuten hasta que el hijo ejecutor termine su trabajo.
          wait(NULL);
        }
      }

      // Se lee del pipe y se pasan estos bytes a la variable output.
      bytesRead = read(pipefd[0], output, sizeof(output) - 1);

      // Si no se leyó nada del pipe (el comando no generó output), se envia el mensaje "Command Executed".
      if (bytesRead > 0)
      {
        output[bytesRead] = '\0';
      }
      else if (bytesRead == -1 && strlen(output) == 0)
      {
        snprintf(output, sizeof(output), "Command executed\n");
      }

      // Se manda el output al cliente.
      send(clientSocket, output, strlen(output), 0);
    }

    // Se cierra el descriptor de lectura del pipe en el proceso hijo original.
    close(pipefd[0]);
  }
  else if (pid > 0) // Padre original.
  {
    // Se cierran los descriptures del pipe en el proceso padre original.
    close(pipefd[0]);
    close(pipefd[1]);
    wait(NULL);
    close(clientSocket);
    close(servSockD);
  }

  return 0;
}