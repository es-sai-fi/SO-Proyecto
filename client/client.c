#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/types.h>

void sendCommand(int sockD, char *input)
{
    while (1)
    {
        printf("Ingrese un comando: ");

        // Se lee el input del usuario y se verifica que no hayan habido errores.
        if (fgets(input, 256, stdin) == NULL)
        {
            perror("Error reading command");
            continue;
        }

        // Se reemplaza el último carácter que es "\n" debido a fegets por uno nulo "\0".
        input[strcspn(input, "\n")] = '\0';

        // Si se digitó algo, se termina el bucle.
        if (strlen(input) != 0)
        {
            break;
        }
    }

    // Se envia el input al servidor.
    send(sockD, input, strlen(input), 0);
}

int main(int argc, char const *argv[])
{
    int sockD = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in servAddr;
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(8080);
    servAddr.sin_addr.s_addr = inet_addr("172.18.0.2");

    int connectStatus = connect(sockD, (struct sockaddr *)&servAddr, sizeof(servAddr));
    if (connectStatus < 0)
    {
        perror("Error connecting to server\n");
        exit(1);
    }

    char input[256];
    char output[1024];

    pid_t pid = fork();
    if (pid == 0)
    {
        int keepRunning = 1;

        while (keepRunning)
        {   
            //Se limpian los buffers.
            memset(input, 0, sizeof(input));
            memset(output, 0, sizeof(output));

            // Se recibe y envia el comando.
            sendCommand(sockD, input);

            // Se recibe el output
            recv(sockD, output, sizeof(output), 0);

            // Se imprime el output
            printf("%s\n", output);

            // Si el input es "salir" se cambia el valor de "continue" a false.
            if (strcmp(input, "salir") == 0)
            {
                keepRunning = 0;
            }
        }
    }
    else if (pid > 0)
    {
        wait(NULL);
        close(sockD);
        printf("Disconnected from server.\n");
    }

    return 0;
}
