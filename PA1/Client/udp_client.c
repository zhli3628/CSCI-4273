#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>
#include <sys/select.h>

#define MAXBUFSIZE 100
#define ACKBUFSIZE 10
#define MESSAGEBUFSIZE 1500

/* You will have to modify the program below */

int main (int argc, char * argv[])
{

	int nbytes;                             // number of bytes send by sendto()
	int sock;                               // this will be our socket
	char buffer[MAXBUFSIZE];				// buffer/packets we will send/receive
	char ackMessage[ACKBUFSIZE] = "ACK";	// ACK that packet was received
	struct sockaddr_in remote;              //"Internet socket address structure"

	const char delim[8] = " \n\r\0"; 		
	char respM[MESSAGEBUFSIZE];			
	char inputfu[MAXBUFSIZE];				
	char Fbuffer[MAXBUFSIZE];				
	long fileSz;								
	FILE *fp;									
	int n;

	if (argc < 3)
	{
		printf("USAGE:  <server_ip> <server_port>\n");
		exit(1);
	}

	/******************
	  Here we populate a sockaddr_in struct with
	  information regarding where we'd like to send our packet 
	  i.e the Server.
	 ******************/
	bzero(&remote,sizeof(remote));               //zero the struct
	remote.sin_family = AF_INET;                 //address family
	remote.sin_port = htons(atoi(argv[2]));      //sets port to network byte order
	remote.sin_addr.s_addr = inet_addr(argv[1]); //sets remote IP address

	//Causes the system to create a generic socket of type UDP (datagram)
	sock = socket(remote.sin_family, SOCK_DGRAM, 0);
	if (sock < 0)
	{
		printf("unable to create socket");
		exit(1);
	}

	// Blocks till bytes are received
	struct sockaddr_in from_addr;
	socklen_t addr_length = sizeof(struct sockaddr);
	bzero(buffer,sizeof(buffer));

	while(1)
	{
		memset(&inputfu, 0, sizeof(inputfu));
		printf("Enter command: ");
		fgets(inputfu,MAXBUFSIZE,stdin);
		char *token = strtok(inputfu, delim);
		char *fileName = strtok(NULL, delim);

		if(strcmp(token, "get") == 0)
		{
			printf("Get file: %s\n", fileName);

			fp = fopen(fileName, "w+b");
			if(fp != NULL)
			{
				n = sprintf(inputfu, "%s %s", token, fileName);
				n = sendto(sock, inputfu, MAXBUFSIZE, 0, (struct sockaddr*)&remote, addr_length);

				int TW = 0;
				fileSz = 0;

				nbytes = recvfrom(sock, &fileSz, sizeof(fileSz),0, (struct sockaddr*)&from_addr, &addr_length);
				
				while(TW < fileSz)
				{
					
					nbytes = recvfrom(sock, Fbuffer, MAXBUFSIZE,0, (struct sockaddr*)&from_addr, &addr_length);
					n = sendto(sock, ackMessage, ACKBUFSIZE, 0, (struct sockaddr*)&remote, addr_length);
					n = fwrite(Fbuffer, MAXBUFSIZE, 1, fp);
					TW += MAXBUFSIZE;
					printf("Have written: %d\n", TW);
					memset(&Fbuffer, 0, MAXBUFSIZE);					
				}

				printf("%d bytes have written to file %s\n", TW, fileName);

				fclose(fp);
			}
			else
			{
				fprintf(stderr, "Unable to open file: %s\n", fileName);
			}

		}
	
		else if (strcmp(token, "put") == 0)
		{
			printf("Put file: %s\n", fileName);
			fp = fopen(fileName, "r+b");
			if(fp != NULL)
			{
				n = sprintf(inputfu, "%s %s", token, fileName);
				n = sendto(sock, inputfu, MAXBUFSIZE, 0, (struct sockaddr*)&remote, addr_length);
				fseek(fp, 0, SEEK_END);
				fileSz = ftell(fp);
				rewind(fp);

				n = sendto(sock, &fileSz, sizeof(fileSz), 0, 
					(struct sockaddr*)&remote, addr_length);
				printf("Sent file size (%ld bytes) to server\n", fileSz);

				int TR = 0;
				int RFL = fileSz;
				while(TR < fileSz)
				{
					fread(Fbuffer, MAXBUFSIZE, 1, fp);
					n = sendto(sock, Fbuffer, MAXBUFSIZE, 0,
						(struct sockaddr*)&from_addr, addr_length);
					if(n == -1)
					{
						printf("Error putting file: %s\n", strerror(errno));
						break;
					}

					else
					{
						recvfrom(sock, ackMessage, ACKBUFSIZE,0, (struct sockaddr*)&from_addr, &addr_length);
					}
					TR += n;
					RFL -= n; 
					printf("Sent %d bytes, %d bytes remaining.\n", n, RFL);
					memset(&Fbuffer, 0, MAXBUFSIZE);				
				}

				fclose(fp);
				memset(&Fbuffer, 0, MAXBUFSIZE); 
			}
			else
			{
				fprintf(stderr, "Unable to open file: %s\n", fileName);
			}
		}

		else if (strcmp(token,"delete") == 0){
			n = sprintf(inputfu, "%s %s", token, fileName);
			n = sendto(sock, inputfu, MAXBUFSIZE, 0, (struct sockaddr*)&remote, addr_length);
			nbytes = recvfrom(sock, respM, MESSAGEBUFSIZE,0, (struct sockaddr*)&from_addr, &addr_length);
			if (strstr(respM,"0")){
				printf("Delete: %s successful\n",fileName);
			}
			else{
				fprintf(stderr, "Unable to delete file: %s\n", fileName);
			}
		}
		else if (strcmp(token, "ls") == 0)
		{
			n = sprintf(inputfu, "%s %s", token, fileName);
			n = sendto(sock, inputfu, MAXBUFSIZE, 0, (struct sockaddr*)&remote, addr_length);
			nbytes = recvfrom(sock, respM, MESSAGEBUFSIZE,0, (struct sockaddr*)&from_addr, &addr_length);
			printf("%s",respM);
		}
		
		else if (strcmp(token, "exit") == 0)
		{
			n = sprintf(inputfu, "%s %s", token, fileName);
			n = sendto(sock, inputfu, MAXBUFSIZE, 0, (struct sockaddr*)&remote, addr_length);

			close(sock);
			exit(1);
		}
		else
		{
			printf("Unknown request\n");

		}
	}

	return 0;
}

