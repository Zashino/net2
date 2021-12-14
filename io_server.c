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

#define REQUEST_SIZE 32384
#define BSIZE 8192
#define MYPORT 7700
#define FD_SIZE 100

struct user{
	char name[256];
	int opponent;
	char game[9];
	int order;
};

void game_board(char game[], char sendbuf[])
{
	int p = sprintf(sendbuf, "┌───┬───┬───┐\n");
	p = sprintf(sendbuf+p, "│ %c │ %c │ %c │\n", game[0], game[1], game[2]) + p;
	p = sprintf(sendbuf+p, "├───┼───┼───┤\n") + p;
	p = sprintf(sendbuf+p, "│ %c │ %c │ %c │\n", game[3], game[4], game[5]) + p;
	p = sprintf(sendbuf+p, "├───┼───┼───┤\n") + p;
	p = sprintf(sendbuf+p, "│ %c │ %c │ %c │\n", game[6], game[7], game[8]) + p;
	p = sprintf(sendbuf+p, "└───┴───┴───┘\n") + p;
}

int game_win(char game[])
{
	if((game[0]==game[1]) && (game[1]==game[2]))
		return 1;
	else if((game[3]==game[4]) && (game[4]==game[5]))
                return 1;
	else if((game[6]==game[7]) && (game[7]==game[8]))
                return 1;
	else if((game[0]==game[3]) && (game[3]==game[6]))
                return 1;
	else if((game[1]==game[4]) && (game[4]==game[7]))
                return 1;
	else if((game[2]==game[5]) && (game[5]==game[8]))
                return 1;
	else if((game[0]==game[4]) && (game[4]==game[8]))
                return 1;
	else if((game[2]==game[4]) && (game[4]==game[6]))
                return 1;

	int j=0;
	for(int i=0; i<9; i++)
		if(!((49<=game[i]) && (game[i]<=57)))
			j++;
	if(j == 9)
		return 2;
}

int main()
{
	int sockfd, newfd, workfd;
	int i, n, maxfd, max_client;
	int nready, clientfd[FD_SIZE];
	fd_set rset, allset;
	struct addrinfo hints, *res;
	struct sockaddr_in their_addr;
	socklen_t addr_size;
	int yes = 1;
	char buf[BSIZE];
	struct user all_user[100];

	bzero(&hints, sizeof(hints));
        hints.ai_family = AF_UNSPEC; // 不用管是 IPv4 或 IPv6
        hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
        hints.ai_flags = AI_PASSIVE; // 幫我填好我的 IP

	if ((getaddrinfo(NULL, "7700", &hints, &res)) != 0) {
                fprintf(stderr, "getaddrinfo error\n");
                exit(1);
        }

	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        	fprintf(stderr, "setsockopt error\n");
        	exit(1);
    	}
       	
        if (bind(sockfd, res->ai_addr, res->ai_addrlen)==-1){
                fprintf(stderr, "bind error\n");
                exit(1);
        }
        freeaddrinfo(res);
        listen(sockfd, 20);

	maxfd = sockfd;
	max_client = -1;
	for (i=0; i<FD_SIZE; i++) {
		clientfd[i] = -1;
	}
	FD_ZERO(&allset);
	FD_SET(sockfd, &allset);


	for ( ; ; ) {
		rset = allset;
		nready = select(maxfd+1, &rset, NULL, NULL, NULL);

		if (FD_ISSET(sockfd, &rset)) {
			addr_size = sizeof(their_addr);
                	newfd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size);
			
			char *strptr;
			strptr = (char *)malloc(100);
			printf("new client: %s, port: %d\n",inet_ntop(AF_INET, &their_addr.sin_addr, strptr, 100), ntohs(their_addr.sin_port));

			printf("fd = %d\n", newfd);
			for (i=0; i<FD_SIZE; i++)
				if (clientfd[i] < 0) {
					clientfd[i] = newfd;
					break;
				}
			if (i == FD_SIZE) {
				fprintf(stderr, "too many clients\n");
				exit(1);
			}

			FD_SET(newfd, &allset);
			if (newfd > maxfd)
				maxfd = newfd;
			if (i > max_client)
				max_client = i;
			
			if (--nready <=0)
				continue;
		}
		for (i=0; i<=max_client; i++) {
			if ((workfd = clientfd[i]) < 0)
				continue;
			if (FD_ISSET(workfd, &rset)) {
				memset(buf, 0, sizeof(buf));
				if ((n = read(workfd, buf, BSIZE)) == 0) {
					close(workfd);
					FD_CLR(workfd, &allset);
					clientfd[i] = -1;
				}
				else if(buf[0] == '+') {
					char *ptr = buf+2;
					strcpy(all_user[workfd].name, ptr);	
				}

				else if(strcmp(buf, "who") == 0) {
					int j;
					char tmp[256], sendbuf[BSIZE];
					memset(sendbuf, 0, sizeof(sendbuf));
					int p = sprintf(sendbuf, "The users:");
					for (j=0; j<100; j++)
                				if (strcmp(all_user[j].name, "") != 0) {
                   				 	sscanf(all_user[j].name,"%s",tmp);
							p = sprintf(sendbuf+p,"%s ", tmp) + p;
                				}
					p = sprintf(sendbuf+p, "\n") + p;
					write(workfd, sendbuf, n);
				}
				else if((buf[0] == 'v')&&(buf[1] == 's')) {
					char *ptr = buf+3, sendbuf[BSIZE];
					int opfd, j;
					memset(sendbuf, 0, sizeof(sendbuf));
					for (j=0; j<100; j++)
                                                if (strcmp(all_user[j].name, ptr) == 0) {
                                               		opfd = j;
					       		break;		
                                                }
					if(j == 100) {
						write(workfd, "沒有這個人\n", 30);
					}
					else{
						if(workfd == opfd) {
							write(workfd, "無法與自己對戰!\n", 30);
							continue;
						}
						all_user[opfd].opponent = workfd;
						all_user[workfd].opponent = opfd;
						strcat(sendbuf, "+1 ");
						strcat(sendbuf, all_user[workfd].name);
						strcat(sendbuf, "想和你一戰 是否同意?[y/n]\n\0");
						write(opfd, sendbuf, 50);
					}
				}
				else if((buf[0] == '-')&&(buf[1]== 'y')) {
					char tmp[BSIZE], sendbuf[BSIZE];
					memset(tmp, 0, sizeof(tmp));
					memset(sendbuf, 0, sizeof(sendbuf));
					write(all_user[workfd].opponent, "對方同意與你對戰\n", 50);

					for (int j=0; j<9; j++) {
						all_user[workfd].game[j] = j+49;
						all_user[all_user[workfd].opponent].game[j] = j+49;
					}
					game_board(all_user[workfd].game, tmp);

					sleep(1);
					strcat(sendbuf, "+2 ");
					strcat(sendbuf, tmp);
					write(all_user[workfd].opponent, sendbuf, BSIZE);

					memset(sendbuf, 0, sizeof(sendbuf));
					strcat(sendbuf, "+3 ");
                                        strcat(sendbuf, tmp);
					write(workfd, sendbuf, BSIZE);
					all_user[workfd].order = 1;
					all_user[all_user[workfd].opponent].order = 2;
				}
				else if((buf[1] == 'n')&&(buf[0]== '-')) {
                                        write(all_user[workfd].opponent, "對方不同意與你對戰\n", 50);
					all_user[workfd].opponent = 0;
                                }
				else if((buf[0]=='-') && (49<=buf[1]) && (buf[1]<=57)) {
					int win_flag = 0;
					char tmp[BSIZE], sendbuf[BSIZE];
					memset(tmp, 0, sizeof(tmp));
					memset(sendbuf, 0, sizeof(sendbuf));
					char *ptr = buf + 1;
					int pattern = atoi(ptr);
					if (all_user[workfd].order == 1) {
						all_user[workfd].game[pattern-1] = 'O';
						all_user[all_user[workfd].opponent].game[pattern-1] = 'O';
					}
					else {
						all_user[workfd].game[pattern-1] = 'X';
						all_user[all_user[workfd].opponent].game[pattern-1] = 'X';
					}

					game_board(all_user[workfd].game, tmp);
					win_flag = game_win(all_user[workfd].game);
					if(win_flag == 1) {
						strcat(sendbuf, tmp);
						strcat(sendbuf, "你贏了!!\n");
						write(workfd, sendbuf, BSIZE);
						
						memset(sendbuf, 0, sizeof(sendbuf));
						strcat(sendbuf, tmp);
                                                strcat(sendbuf, "你輸了!!\n");
                                                write(all_user[workfd].opponent, sendbuf, BSIZE);
						continue;
					}
					else if(win_flag == 2) {
						strcat(sendbuf, tmp);
                                                strcat(sendbuf, "平手!!\n");
                                                write(workfd, sendbuf, BSIZE);
						write(all_user[workfd].opponent, sendbuf, BSIZE);
                                                continue;
					}

					strcat(sendbuf, "+2 ");
					strcat(sendbuf, tmp);
					write(all_user[workfd].opponent, sendbuf, BSIZE);

					memset(sendbuf, 0, sizeof(sendbuf));
                                        strcat(sendbuf, "+3 ");
                                        strcat(sendbuf, tmp);
                                        write(workfd, sendbuf, BSIZE);
				}

				else {
					printf("recieve: %s",buf);
				}
				if (--nready <= 0)
					break;
			}
		}       		       
	}
}
