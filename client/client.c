#include <netinet/in.h>
#include <stdio.h> 
#include <stdlib.h> 
#include <sys/socket.h>  
#include <sys/types.h> 

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

	else { 
		char strData[255]; 

		recv(sockD, strData, sizeof(strData), 0); 

		printf("Message: %s\n", strData); 
	} 

	return 0; 
}
