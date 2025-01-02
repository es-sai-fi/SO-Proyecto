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
    while(1){
        printf("Ingrese un comando: ");

        //Se lee el input del usuario y se verifica que no hayan habido errores.
        if (fgets(input, 256, stdin) == NULL) {
            perror("Error reading command");
            continue;
        }

        //Se reemplaza el último carácter que es "\n" debido a fegets por uno nulo "\0".
        input[strcspn(input, "\n")] = '\0';

        //Si se digitó algo, se termina el bucle.
        if (strlen(input) != 0) {
            break;
        }
    }
    
    //Se envia el input al servidor.
    send(sockD, input, strlen(input), 0);
}

int main(int argc, char const* argv[]) {
    int sockD = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in servAddr;
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(8080);
    servAddr.sin_addr.s_addr = inet_addr("172.18.0.2");

    int connectStatus = connect(sockD, (struct sockaddr*)&servAddr, sizeof(servAddr));
    if (connectStatus < 0) {
        perror("Error connecting to server\n");
        exit(1);
    }

    char input[256];
    char output[1024];

    int pipefd[2];
    if (pipe(pipefd) < 0) {
        perror("Error creating pipe");
        exit(1);
    }

    while (1) {
        memset(input, 0, sizeof(input));
        memset(output, 0, sizeof(output));

        pid_t pid = fork();
        if (pid == 0) {
            //Se cierra la apertura de lectura.
            close(pipefd[0]);

            //Se recibe y envia el comando.
            sendCommand(sockD, input);
            //Se escribe en la apertura de escritura.
            write(pipefd[1], input, sizeof(input));
            //Se cierra la apertura de escritura.
            close(pipefd[1]);

            //Se recibe el output
            recv(sockD, output, sizeof(output), 0);
            printf("%s\n", output);

            exit(0);
        } else if (pid > 0) {
            size_t bytesRead;

            wait(NULL);

            //Se cierra la apertura de escritura.
            close(pipefd[1]);
            //Se lee de la apertura de lectura y se pasa la información a input.
            bytesRead = read(pipefd[0], input, sizeof(input)-1);
            //Se cierra la apertura de lectura.
            close(pipefd[0]);

            input[bytesRead] = '\0';

            //Si el comando fue "salir", se rompe el ciclo.
            if (strcmp(input, "salir") == 0) {
                break;
            }
        }
    }
	
	close(sockD);
	printf("Disconnected from server.\n");

	return 0; 
}
