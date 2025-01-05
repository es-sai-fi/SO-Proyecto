#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <fcntl.h>
#include <assert.h>

// La función es exactamente la misma que de_vector_a_cadena en el proyecto minishell del profesor, créditos hacia su persona.
char **processInput(char *input)
{
  int i;
  char *token;       // Variable para almacenar cada token extraído mediante strtok.
  char *delim = " "; // Delimitador en base al cual se van a separar las tokens.
  char **resultado;

  // Se asigna inicialmente cierto espacio a resultado.
  resultado = (char **)malloc(sizeof(char *));
  assert(resultado != NULL); // Se verifica que fue exitosa la asignación.
  i = 0;
  token = strtok(input, delim); // Se extrae el primer token.
  while (token != NULL)         // Este bucle seguira funcionando hasta que no haya tokens por extraer.
  {
    int cad_longitud;
    char **result_temp;
    cad_longitud = strlen(token) + 1;
    // Copiar token en la posicion 'i'
    resultado[i] = strdup(token);
    // printf("|%s|\n",resultado[i]); fflush(stdout);
    //  En busca de la proxima cadena
    token = strtok(NULL, delim); // Se extrae la siguiente token.
    i++;
    result_temp = realloc(resultado, (i + 1) * sizeof(*resultado)); // Se aumenta el espacio de result_temp por cada ciclo.
    assert(result_temp != NULL);
    resultado = result_temp; // Por lo pronto el resultado final será result_temp.
  }

  resultado[i] = NULL; // Nos aseguramos que el último elemento sea NULL para que execvp funcione.
  return resultado;

  // Es un poco complicada la función porque se está trabajando con un tamaño de vector indefinido por lo tanto por cada token escrito se realoca memoria para guardar esa token, es decir, básicamente estamos convirtiendo cadenas de longitud desconocida sin saber su número de tokens (palabras separadas por espacios) en un vector char de tamaño dinámico (mediante realloc).
}

void processInputCd(char input[], char **args)
{
  args[0] = "cd";

  // Dividimos entonces la cadena en input después de los 3 primeros carácteres hasta el final de la cadena.
  args[1] = strtok(input + 3, "");

  args[2] = NULL;
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

  // Vector donde se guardará la cadena convertida en vector.
  char **args;

  size_t bytesReceived, bytesRead;

  int keepRunning = 1;

  while (keepRunning)
  {
    // Nos aseguramos de que los buffers estén "vacíos".
    memset(input, 0, sizeof(input));
    memset(output, 0, sizeof(output));

    // Se crea un pipe para redigir el output y cualquier error ocurrido al padre (se establece como no bloqueante).
    int pipefd[2];
    pipe(pipefd);

    // Se recibe la cadena.
    bytesReceived = recv(clientSocket, input, sizeof(input), 0);

    // Se convierte el carácter siguiente al último del input a uno nulo. Esto es importante para funciones como strtok.
    input[bytesReceived] = '\0';

    // Si los primeros dos carácteres de input son cd y el tercero es " " o un carácter nulo entonces se asume que es solicitud de cd.
    if (strncmp(input, "cd", 2) == 0 && (input[2] == ' ' || input[2] == '\0'))
    {
      // Se ejecuta otra versión de processInput, esta es para solicitudes de "cd".
      processInputCd(input, args);
    }
    else
    {
      // Se ejecuta la función processInput que se encargará de dividir la cadena en elementos de un vector acorde a " ".
      args = processInput(input);
    }

    // Se crea un nuevo proceso para atender el comando a ejecutar.
    pid_t pid = fork();

    // Proceso hijo.
    if (pid == 0)
    {
      // Se cierra la entrada de lectura del hijo.
      close(pipefd[0]);
      // Se redirige el output y cualquier error que genere el hijo hacia el pipe.
      dup2(pipefd[1], STDOUT_FILENO);
      dup2(pipefd[1], STDERR_FILENO);
      // Se cierra la entrada de escritura del hijo.
      close(pipefd[1]);

      // Si la cadena recibida es "salir" o el primer arg de args es "cd" nos aseguramos que el padre se encargue de estas solicitudes.
      if (strcmp(args[0], "salir") == 0 || strcmp(args[0], "cd") == 0)
      {
        exit(0);
      }
      // Se ejecuta el comando mediante execvp.
      else
      {
        // Si execvp falla imprimimos el error.
        if (execvp(args[0], args) == -1)
        {
          perror("Execvp failed");
          exit(1);
        }
      }
    }
    // Proceso padre.
    else if (pid > 0)
    {
      // Se espera a que el proceso hijo termine.
      wait(NULL);

      // Si la cadena recibida es "salir" entonces se cambia el valor de "continue" a false.
      if (strcmp(input, "salir") == 0)
      {
        const char *message = "Saliendo...\n";
        write(pipefd[1], message, strlen(message));
        keepRunning = 0;
      }
      // Si el comando principal es cd se utiliza chdir.
      else if (strcmp(args[0], "cd") == 0)
      {
        if (args[1] == NULL)
        {
          const char *message = "No directory specified\n";
          write(pipefd[1], message, strlen(message));
        }
        else
        {
          if (chdir(args[1]) != 0)
          {
            const char *message = "Chdir failed\n";
            write(pipefd[1], message, strlen(message));
          }
        }
      }

      // Se cierra la entrada de escritura del padre.
      close(pipefd[1]);

      // Se lee del pipe y se pasan estos bytes a la variable output.
      bytesRead = read(pipefd[0], output, sizeof(output) - 1);

      // Se cierra la entrada de lectura del padre.
      close(pipefd[0]);

      // Si no se leyó nada del pipe (el comando no generó output), se envia " ".
      if (bytesRead == 0)
      {
        snprintf(output, sizeof(output), " ");
      }
      else
      {
        output[bytesRead] = '\0';
      }

      // Se manda el output al cliente.
      send(clientSocket, output, strlen(output), 0);
    }
  }

  close(clientSocket);
  close(servSockD);

  return 0;
}