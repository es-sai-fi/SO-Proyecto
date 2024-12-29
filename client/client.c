#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/types.h>

void sendCommand(int sockD, char *input) {
    printf("Ingrese un comando: ");
    fflush(stdout);  // Asegura que el mensaje se imprima de inmediato

    if (fgets(input, 256, stdin) == NULL) {
        perror("Error reading command");
        return;
    }

    // Remover el salto de línea al final
    size_t len = strlen(input);
    if (len > 0 && input[len - 1] == '\n') {
        input[len - 1] = '\0';
    }

    // Verificar si el comando está vacío
    if (strlen(input) == 0) {
        printf("No se ingresó un comando.\n");
        return;
    }

    // Enviar comando al servidor
    if (send(sockD, input, strlen(input), 0) < 0) {
        perror("Error enviando el comando");
    }
}

int main(int argc, char const* argv[]) {
    int sockD = socket(AF_INET, SOCK_STREAM, 0);
    if (sockD < 0) {
        perror("Error creando socket");
        exit(1);
    }

    struct sockaddr_in servAddr;
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(8080);
    servAddr.sin_addr.s_addr = inet_addr("172.18.0.2");

    if (connect(sockD, (struct sockaddr*)&servAddr, sizeof(servAddr)) < 0) {
        perror("Error conectándose al servidor");
        exit(1);
    }
    printf("Conectado al servidor.\n");

    char input[256];
    char output[1024];

    while (1) {
        // Leer y enviar comando
        sendCommand(sockD, input);

        // Salir si el comando es "salir"
        if (strcmp(input, "salir") == 0) {
            break;
        }

        // Recibir respuesta del servidor
        int bytesReceived = recv(sockD, output, sizeof(output) - 1, 0);
        if (bytesReceived < 0) {
            perror("Error recibiendo datos");
            break;
        }

        // Asegurar que el output sea una cadena válida
        output[bytesReceived] = '\0';

        // Imprimir la respuesta
        printf("%s\n", output);
    }

    close(sockD);
    printf("Desconectado del servidor.\n");
    return 0;
}
