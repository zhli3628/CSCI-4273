#include <sys/types.h>
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
#include <string.h>
#include <dirent.h>

/* You will have to modify the program below */

#define MAXBUFSIZE 100
#define ACKBUFSIZE 20
#define MESSAGEBUFSIZE 1500
int main (int argc, char * argv[] )
{


	int sock;                           //This will be our socket
	struct sockaddr_in sin, remote;     //"Internet socket address structure"
	unsigned int remote_length;         //length of the sockaddr_in structure
	int nbytes;                        //number of bytes we receive in our message
	char buffer[MAXBUFSIZE];             //a buffer to store our received message
	char ackMessage[ACKBUFSIZE] = "ACK";
	int n;
	long fileSz;
	char respM[MESSAGEBUFSIZE];
	char Fbuffer[MAXBUFSIZE];
	FILE *fp;
	DIR *dirp;
	const char delim[8] = " \0\n\r";
	if (argc != 2)
	{
		printf ("USAGE:  <port>\n");
		exit(1);
	}

	/******************
	  This code populates the sockaddr_in struct with
	  the information about our socket
	 ******************/
	bzero(&sin,sizeof(sin));                    //zero the struct
	sin.sin_family = AF_INET;                   //address family
	sin.sin_port = htons(atoi(argv[1]));        //htons() sets the port # to network byte order
	sin.sin_addr.s_addr = INADDR_ANY;           //supplies the IP address of the local machine


	//Causes the system to create a generic socket of type UDP (datagram)
	sock = socket(sin.sin_family, SOCK_DGRAM, 0);
	if (sock < 0)
	{
		printf("unable to create socket");
		exit(1);
	}


	/******************
	  Once we've created a socket, we must bind that socket to the 
	  local address and port we've supplied in the sockaddr_in struct
	 ******************/
	if (bind(sock, (struct sockaddr *)&sin, sizeof(sin)) < 0)
	{
		printf("unable to bind socket\n");
		exit(1);
	}
	remote_length = sizeof(remote);

	while(1){
		//waits for an incoming message
		bzero(buffer,sizeof(buffer));
		printf("The client says %s\n", buffer);
		nbytes  = recvfrom(sock, buffer, MAXBUFSIZE, 0,
                          (struct sockaddr *)&remote, &remote_length);
		char *token = strtok(buffer,delim);
		char *fileNm = strtok(NULL, "\n\r");
		if(strcmp(token,"get") == 0){
			printf("Get request for file: %s\n", fileNm);
			fp = fopen(fileNm,"r+b");
			if(fp != NULL){
				fseek(fp,0,SEEK_END);
				fileSz = ftell(fp);
				rewind(fp);
				n = sendto(sock, &fileSz, sizeof(fileSz), 0, 
					(struct sockaddr*)&remote, remote_length);
				int HR = 0; //have read
				int RFL = fileSz;
				while(HR < fileSz){
					fread(Fbuffer, MAXBUFSIZE,1,fp);
					n = sendto(sock, Fbuffer, MAXBUFSIZE, 0,
							(struct sockaddr*)&remote, remote_length);
					if(n == -1){
						printf("Error sending file: %s\n", strerror(errno));
							break;
						}
					else{
						recvfrom(sock, ackMessage, ACKBUFSIZE,0, (struct sockaddr*)&remote, &remote_length);
					}
					HR += n;
					RFL -= n;
					memset(&Fbuffer, 0, MAXBUFSIZE);
			}
				fclose(fp);
				memset(&Fbuffer, 0, MAXBUFSIZE);
			}
			else{
				fprintf(stderr, "Fail to open file: %s\n",fileNm);
				nbytes = sendto(sock, stderr, MAXBUFSIZE, 0, 
					(struct sockaddr*)&remote, remote_length);
			}	
		}
		else if (strcmp(token,"put") == 0){
			printf("Put request for file: %s\n", fileNm );
			fp = fopen(fileNm,"w+b");
			if (fp != NULL){
				long HW = 0;
				fileSz = 0;
				nbytes = recvfrom(sock, &fileSz, sizeof(fileSz),0,
						 (struct sockaddr*)&remote, &remote_length);
				while (HW<fileSz){
					nbytes = recvfrom(sock, Fbuffer, MAXBUFSIZE,0,(struct sockaddr*)&remote, &remote_length);
					n = sendto(sock, ackMessage, ACKBUFSIZE, 0, (struct sockaddr*)&remote, remote_length);
					n = fwrite(Fbuffer,MAXBUFSIZE,1,fp);
					HW += MAXBUFSIZE;
					printf("Have writtern: %ld\n",HW);
					memset(&Fbuffer,0,MAXBUFSIZE);
				}
				fclose(fp);
			}
			else{
				fprintf(stderr, "Fail to open file: %s\n",fileNm);
				nbytes = sendto(sock, stderr, MAXBUFSIZE, 0, 
					(struct sockaddr*)&remote, remote_length);
			}
			
		}
		else if (strcmp(token,"delete") == 0){
			printf("Delete request for file: %s\n", fileNm);
			int ret;
			ret = remove(fileNm);
			if (ret == 0){
				printf("Delete: %s successful\n",fileNm);
				sprintf(respM,"%d",ret);
				n = sendto(sock,respM,MESSAGEBUFSIZE,0,
					(struct sockaddr*)&remote, remote_length);
			}
			else{
				fprintf(stderr, "Fail to delete file: %s\n",fileNm);
				nbytes = sendto(sock, stderr, MAXBUFSIZE, 0, 
					(struct sockaddr*)&remote, remote_length);
			}
			memset(&respM, 0, MESSAGEBUFSIZE);
		}
		else if (strcmp(token, "ls") == 0){
			int len = 0;
			struct dirent *dp;
			dirp = opendir(".");
			if (dirp != NULL){
				while((dp = readdir(dirp)) != NULL){
					len += sprintf(respM + len,"%s\n",dp->d_name);
				}
				n = sendto(sock,respM,MESSAGEBUFSIZE,0,
					(struct sockaddr*)&remote, remote_length);
				if (n == -1){
					printf("Error sending ls: %s\n", strerror(errno));
					break;
				}
				closedir(dirp);
			}
			else{
				fprintf(stderr, "Unable to use ls command.\n");
			}
			memset(&respM, 0, MESSAGEBUFSIZE);
		}

		else if (strcmp(token,"exit")==0){
			close(sock);
			exit(1);
		}
		else{
			printf("Unknown request\n");
		}
	}
	return 0;
}

