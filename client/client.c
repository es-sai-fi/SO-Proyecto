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
  if (fgets(input, 256, stdin) == NULL) {
    perror("Error reading command");
    return;
  }
  input[strcspn(input, "\n")] = '\0';
  send(sockD, input, strlen(input), 0);
}

int main(int argc, char const* argv[]) 
{ 
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
	printf("Connected to server.\n");

	char input[256];
	char output[1024];

	while (1) {
		//Se crea un nuevo proceso para enviar el comando al servidor.
		pid_t pid = fork();

		//Proceso hijo.
		if (pid == 0) {
			//Se envia el comando mediante sendCommand.
			sendCommand(sockD, input);

			//Si la cadena enviada es "salir" entonces termina el proceso hijo.
			if (!strcmp(input, "salir")) {
				exit(0);
			}
			
			//Se recibe el output del comando desde el servidor.
			recv(sockD, output, sizeof(output), 0);

			printf(output);

			exit(0);
		} 
		//Proceso padre.
		else if (pid > 0) {
			wait(NULL);

			//Si la cadena enviada es "salir" entonces se rompe el bucle.
			if (!strcmp(input, "salir")) {
				break;
			}
		} 
		else {
			perror("Error creating child process");
		}
	}
	
	close(sockD);
	printf("Disconnected from server.\n");

	return 0; 
}
