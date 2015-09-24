/******************************************************************/
/* Minimal BVH motion line send server                            */
/* -------------------------------------------------------------- */
/* Copyright 2015 Heiko Fink, All Rights Reserved.                */
/*                                                                */
/* This file is subject to the terms and conditions defined in    */
/* file 'LICENSE.txt', which is part of this source code package. */
/*                                                                */
/* Description:                                                   */
/* ------------                                                   */
/* Reads in a BVH motion file, opens a listening TCP socket and   */
/* sends each motion line to the connected clients.               */
/* The delay between the motion lines can be configured or        */
/* is read from the BVH file.                                     */
/* If all motion lines have been sent the server loops back to    */
/* the first one.                                                 */
/******************************************************************/

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>


#define PROGNAME    "bvhsend"
#ifdef DEBUG
#define MODE        "debug"
#else
#define MODE        "runtime"
#endif
#define VERS        "0.1.0"
#define DATE        "24-SEP-2015"

void help(void)
{
  printf("\n%s  %s-%s                              %s\n", PROGNAME, VERS, MODE, DATE);
  printf("---------------------------------------------------------------\n");
  printf("Usage: %s port frametime format bvhfile\n", PROGNAME);
  printf("  Ex.: %s 7001 10000 0 example.bvh\n", PROGNAME);
  printf("port     : TCP port number.\n");
  printf("frametime: Delay between each motion line in microseconds.\n");
  printf("           Set to 0 if frame time from BVH file should be used.\n");
  printf("format   : 0 = Just send complete line as it is in BVH file\n");
  printf("           1 = Use Axis Neuron format\n");
  printf("bvhfile  : Name and path of BVH file to be sent.\n");
  printf("---------------------------------------------------------------\n");
  printf("Heiko Fink                                         hfink@web.de\n");
}


#define MAX_TCP_CONNECTION	5
#define MAX_FILENAME_LEN	200
unsigned long read_next_motion_line(unsigned char *bvhdata, unsigned long bvhdata_size, unsigned char **data);
int read_frametime(unsigned char *bvhdata, unsigned long bvhdata_size, unsigned long *frametime);
unsigned long ufloatstr2long(unsigned char *data);
ssize_t send_motion_line(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen, int format);

int exit_flag = 0;
void sigaction_handler(int signal)
{
	exit_flag = 1;
}

int main(int argc, char**argv)
{
	int socket_fd;
	int conn_fd;
	int port;
	struct sockaddr_in server_addr;
	struct sockaddr_in client_addr;
	socklen_t client_addr_len;
	struct sigaction action;
	FILE *bvhfile_fd;
	char bvhfile_name[MAX_FILENAME_LEN];
	struct stat bvhfile_stat;
	unsigned long bvhfile_size;
	unsigned char *bvhdata, *data;
	unsigned long length;
	unsigned long motionline_delay;
	int format;
	
	/* Scan command line arguments */
	if(5 != argc){
		help();
		return(-1);
	}	
	port = atoi(argv[1]);		
	motionline_delay = atol(argv[2]);
	format = atoi(argv[3]);		
	strncpy(bvhfile_name, argv[4], sizeof(bvhfile_name));
	
	/* Read BVH file */
	if((bvhfile_fd = fopen(bvhfile_name, "r+b")) == NULL){
		printf("Can not open file :\"%s\"\n", bvhfile_name);
		return(-1);
	}
	stat(bvhfile_name, &bvhfile_stat);
	bvhfile_size = bvhfile_stat.st_size;
	if((bvhdata = malloc(bvhfile_size)) == NULL){
		printf("Can not allocate \"%ld\" bytes\n", bvhfile_size);
		fclose(bvhfile_fd);
		return(-1);	
	}
	if(fread(bvhdata, sizeof(char), bvhfile_size, bvhfile_fd) != bvhfile_size){
		printf("Read failed\n");
		fclose(bvhfile_fd);
		free(bvhdata);
		return(-1);	
	}
	fclose(bvhfile_fd);
	
	/* Create and open a server TCP socket */
	socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(-1 == socket_fd){
		printf("Could not create TCP socket\n");
		free(bvhdata);
		return(-1);	
	}	
	memset(&server_addr, 0, sizeof(server_addr));   
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(port);   
	if(bind(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1){
		printf("Could not bind TCP socket\n");
		free(bvhdata);
		close(socket_fd);
		return(-1);	
	}
	if(listen(socket_fd, MAX_TCP_CONNECTION) == -1){
		printf("Could not listen on TCP socket\n");
		free(bvhdata);
		close(socket_fd);
		return(-1);	
	}

	/* Use frame time delay from BVH file ? */
	if(motionline_delay == 0){
		if(read_frametime(bvhdata, bvhfile_size, &motionline_delay) == -1){
			printf("Could not read frame time\n");
			free(bvhdata);
			close(socket_fd);
			return(-1);	
		}
		printf("Frametime: %u\n", motionline_delay);
	}
	
	/* Catch SIGINT signal (Ctrl+C) to close sockets gracefully */
	memset (&action, '\0', sizeof(action));
	action.sa_handler = &sigaction_handler;
	if (sigaction(SIGINT, &action, NULL) < 0){
		printf("Could not set SIGINT to close sockets gracefully\n");
		free(bvhdata);
		close(socket_fd);
		return(-1);	
	}

	/* Infinite server loop */
	while(1){
		client_addr_len = sizeof(client_addr);
		conn_fd = accept(socket_fd, (struct sockaddr *)&client_addr, &client_addr_len);
		if(-1 != conn_fd){		
			if(0 == fork()){
				/* New child process */		
				close(socket_fd);		
		
				while(1){
					length = read_next_motion_line(bvhdata, bvhfile_size, &data);
					if((0L == length) || (exit_flag)){
						/* Exit child process */
						free(bvhdata);
						close(conn_fd);
						return(0);
					}
					if(send_motion_line(conn_fd, data, length, 0, (struct sockaddr *)&client_addr, sizeof(client_addr), format) == -1){
						/* Exit child process on send error */
						free(bvhdata);
						close(conn_fd);
						return(0);
					}						
					usleep(motionline_delay); /* microseconds */
				}
			} /* End fork */
			close(conn_fd);
		}
		if(exit_flag){
			/* Exit parent process */
			free(bvhdata);
			close(socket_fd);
			return(0);
		}
	} /* End server loop */
} /* End main */

ssize_t send_motion_line(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen, int format)
{
	ssize_t size;

	switch(format)
	{
		default:
		case 0:	/* Send line as it is */
			if((size = sendto(sockfd, buf, len, flags, dest_addr, addrlen)) == -1)
				return -1;		
			break;
		case 1:	/* Axis Neuron format */		
#define NEURON_BEGIN_LINE "0 Avatarname "
			if((size = sendto(sockfd, NEURON_BEGIN_LINE, sizeof(NEURON_BEGIN_LINE), flags, dest_addr, addrlen)) == -1)
				return -1;	
			if((size = sendto(sockfd, buf, len-1, flags, dest_addr, addrlen)) == -1)
				return -1;
#define NEURON_END_LINE " ||\r\n"
			if((size = sendto(sockfd, NEURON_END_LINE, sizeof(NEURON_END_LINE), flags, dest_addr, addrlen)) == -1)
				return -1;
			break;
	}
	
	return size;
}

unsigned long read_next_motion_line(unsigned char *bvhdata, unsigned long bvhdata_size, unsigned char **data)
{
	static unsigned long first_motion_line = 0L;
	static unsigned long next = 0L;
	unsigned long i, length = 0L;
	
	/* First time ? */
	if(0L == first_motion_line){
		/* Forward till motion block */
		for(i = 0; i < (bvhdata_size-10); i++){
			if((bvhdata[i] == 'F') && (bvhdata[i+1] == 'r') && (bvhdata[i+2] == 'a') 
				&& (bvhdata[i+3] == 'm') && (bvhdata[i+4] == 'e') && (bvhdata[i+5] == ' ') 
				&& (bvhdata[i+6] == 'T') && (bvhdata[i+7] == 'i') && (bvhdata[i+8] == 'm') 
				&& (bvhdata[i+9] == 'e'))
				break;
		}
		for(; i < bvhdata_size; i++){
			if((bvhdata[i] == '\n') || (bvhdata[i] == '\r'))
				break;
		}
		for(; i < bvhdata_size; i++){
			if((bvhdata[i] == '-') || ((bvhdata[i] >= '0') && (bvhdata[i] <= '9')))
				break;
		}
		if(i >= bvhdata_size)
			return 0L;
		first_motion_line = i;
		next = first_motion_line;
	}
	
	/* Give back next line */
	*data = &bvhdata[next];	
	for(i = next; i < bvhdata_size; i++){
		if((bvhdata[i] == '\n') || (bvhdata[i] == '\r'))
			break;
	}
	length = (i - next) + 1;
	
	/* Setup next line for next call and check if we are at the end of the file => Loop back to begin. */
	for(; i < bvhdata_size; i++){
		if((bvhdata[i] == '-') || ((bvhdata[i] >= '0') && (bvhdata[i] <= '9')))
			break;
	}
	if(i >= bvhdata_size)
		next = first_motion_line;
	else
		next = i;
			
	return length;
} /* End read_next_motion_line */

int read_frametime(unsigned char *bvhdata, unsigned long bvhdata_size, unsigned long *frametime)
{
	unsigned long i;
	
	/* Search for "Frame Time" */
	for(i = 0; i < (bvhdata_size-10); i++){
		if((bvhdata[i] == 'F') && (bvhdata[i+1] == 'r') && (bvhdata[i+2] == 'a') 
			&& (bvhdata[i+3] == 'm') && (bvhdata[i+4] == 'e') && (bvhdata[i+5] == ' ') 
			&& (bvhdata[i+6] == 'T') && (bvhdata[i+7] == 'i') && (bvhdata[i+8] == 'm') 
			&& (bvhdata[i+9] == 'e'))
			break;
	}
	for(; i < bvhdata_size; i++){
		if((bvhdata[i] >= '0') && (bvhdata[i] <= '9'))
			break;
	}
	if(i >= bvhdata_size)
		return -1;
		
	*frametime = ufloatstr2long(&bvhdata[i]);	
	return 0;
}

/* Converts a float string to a long variable 
and uses maximal 6 positions after decimal point. */
unsigned long ufloatstr2long(unsigned char *data)
{
/* Maximal long size in decimal */
#define MAX_ARRAY 20
	unsigned char temp[MAX_ARRAY];
	int i, j, decimalpoint = 0, rightpos = 0;
	
	memset(temp, 0, sizeof(temp));
	
	for(i = j = 0; (i < MAX_ARRAY-1) && (rightpos < 6); i++){
		if(data[i] == '.'){
			decimalpoint = 1;
		}
		else if((data[i] >= '0') && (data[i] <= '9')){
			temp[j] = data[i]; 
			j++;
			if(decimalpoint)
				rightpos++;
		}
		else
			break;
	}
	for(;(j < MAX_ARRAY-1) && (rightpos < 6); rightpos++){
		temp[j] = '0';
		j++;
	}
	
	return(atol(temp));
}
