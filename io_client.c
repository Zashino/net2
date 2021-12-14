#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>

#define BSIZE 8192
#define PORT 7700

int max(int a, int b)
{
	if (a > b)
		return a;
	else
		return b;
}

int main(int argc, char **argv)
{
	int sockfd, n, maxfd, nready;
	fd_set rset;
	FILE *fp = stdin;
	char buf[BSIZE], recvbuf[BSIZE], name[256];
	struct sockaddr_in servaddr;

	if (argc != 2) {
		fprintf(stderr, "Incorrect input!\n");
		exit(1);
	}

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		fprintf(stderr, "Socket error!\n");
		exit(1);
	}

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(PORT);
	if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0) {
		fprintf(stderr, "Inet_pton error!\n");
		exit(1);
	}

	if(connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    	{
        	fprintf(stderr, "Connect error!\n");
        	exit(1);
    	}
	printf("加入成功！\n");
	
	printf("請輸入名字：\n");
	fgets(name, sizeof(name), stdin);

	char *qtr;
	if((qtr = strstr(name, "\n")))
                *qtr = '\0';
	printf("Welcome %s!\n", name);
	
	char user[256];
	strcat(user, "+ ");
	strcat(user, name);
	write(sockfd, user, sizeof(user));
	for ( ; ; ) {
		FD_SET(fileno(fp), &rset);
		FD_SET(sockfd, &rset);
		maxfd = max(fileno(fp), sockfd) + 1;
		nready = select(maxfd, &rset, NULL, NULL, NULL);

		if(FD_ISSET(fileno(fp), &rset)) {
			memset(buf, 0, sizeof(buf));
        		fgets(buf, sizeof(buf), stdin);
			if (strcmp(buf, "logout\n") == 0) {
            			printf("離開遊戲\n");
            			break;
        		}
			if ((buf[0] == 'v')&&(buf[1] == 's')) {
				printf("等待對方回應...\n");
				char *ptr=buf;
                        	if((ptr = strstr(buf, "\n")))
                                	*ptr = '\0';
                        	write(sockfd, buf, sizeof(buf));
				
				memset(recvbuf, 0, sizeof(recvbuf));
				if(read(sockfd, recvbuf, BSIZE) == 0) {
                                	fprintf(stderr, "Server terminated!\n");
                                	exit(1);
                        	}
				fputs(recvbuf, stdout);
				continue;
			}

			char *ptr=buf;
			if((ptr = strstr(buf, "\n")))
				*ptr = '\0';
			write(sockfd, buf, sizeof(buf));
			//printf("write already write %s\n",buf);
		}
		if(FD_ISSET(sockfd, &rset)) {
			if(read(sockfd, recvbuf, BSIZE) == 0) {
				fprintf(stderr, "Server terminated!\n");
				exit(1);
			}
			if((recvbuf[0] == '+') && (recvbuf[1] == '1')) {
				char *ptr = recvbuf + 3;
				fputs(ptr, stdout);
				
				memset(buf, 0, sizeof(buf));
                        	while(fgets(buf, sizeof(buf), stdin)) {
					if (strcmp(buf, "y\n") == 0) {
                                		write(sockfd, "-y", 20);
						break;
                        		}
					else if (strcmp(buf, "n\n") == 0) {
						write(sockfd, "-n", 20);
						break;
					}
					else {
						printf("請輸入y/n:\n");
					}
				}
			}
			else if((recvbuf[0] == '+') && (recvbuf[1] == '2')) {
				char *ptr = recvbuf + 3;
                                fputs(ptr, stdout);
				printf("輪到你了 請輸入1~9\n");
				
				memset(buf, 0, sizeof(buf));
				while(fgets(buf, sizeof(buf), stdin)) {
					int input = atoi(buf);
                                        if ((input>0) && (input<10)) {
						char sendbuf[128];
						memset(sendbuf, 0, sizeof(sendbuf));
						strcat(sendbuf, "-");
						strcat(sendbuf, buf);
                                                write(sockfd, sendbuf, 128);
                                                break;
                                        }
                                        else {
                                                printf("請輸入1~9\n");
                                        }
                                }
			}
			else if((recvbuf[0] == '+') && (recvbuf[1] == '3')) {
                                char *ptr = recvbuf + 3;
                                fputs(ptr, stdout);
                                printf("等待對方回應...\n");
                        }
			else 
				fputs(recvbuf, stdout);
		}
	}
	close(sockfd);
}
