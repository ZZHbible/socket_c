/********************************************************************
*   File Name: client.c
*   Description: 用于实现多客户端通信
*   Others: 
*     1. 客户端只需考虑如何连接目标服务器即可
*     2. 客户端首先创建套接字并向服务器附送网络连接请求
*     3. 连接到服务器之后，创建子进程
*     4. 父进程用来向服务器发送数据，子进程从服务器接收数据
*********************************************************************/
#include "mysocket.h"

int main(int argc, char *argv[]){
	int client_sockfd;
	int len;
	pid_t pid;
    char buf_recv[BUFSIZ];                                 	//数据传送的缓冲区
	char buf_send[BUFSIZ];
	struct sockaddr_in remote_addr;                         //服务器端网络地址结构体

	/* 初始化目标服务器的网络信息 */
	memset(&remote_addr, 0, sizeof(remote_addr));           //数据初始化--清零
	remote_addr.sin_family = AF_INET;                       //设置为IP通信
	remote_addr.sin_addr.s_addr = inet_addr(SERVER_IP);     //服务器IP地址
	remote_addr.sin_port = htons(SERVER_PORT);              //服务器端口号
	
	/*创建客户端套接字--IPv4协议，面向连接通信，TCP协议*/
	if((client_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		perror("socket error");
		return 1;
	}
	printf("client_sockfd:%d\n",&client_sockfd);
	
	/*将套接字绑定到服务器的网络地址上*/
	if(connect(client_sockfd, (struct sockaddr *)&remote_addr, sizeof(struct sockaddr)) < 0){
		perror("connect error");
		return 1;
	}
	printf("connected to server\n");

	/* 从服务器接收初始化的握手消息 */
	len = recv(client_sockfd, buf_recv, BUFSIZ, 0);			//接收服务器端信息
    buf_recv[len] = '\0';
	printf("%s", buf_recv);									//打印服务器端的欢迎信息
	printf("Enter string to send: \n");
	
	/* 创建父子进程与服务器进行通信 */
	if((pid = fork()) < 0){
		printf("Fail to call fork()\n");
		return 1;
	}
	
	/* 父进程用来发送数据 */
	else if(pid > 0){
		int len=0;
		while(1){
			// scanf("%s", buf_send);
			fgets(buf_send,sizeof(buf_send),stdin);
			if(!strcmp(buf_send, "quit")){
				kill(pid, SIGSTOP);
				break;
			}
			len = send(client_sockfd, buf_send, strlen(buf_send), 0);
		}
	}
	/* 子进程用来接收数据 */
	else{
		while(1){
			memset(buf_recv, 0, sizeof(buf_recv));
			if((len = recv(client_sockfd, buf_recv, BUFSIZ, 0)) > 0){
				printf("Recive from server: %s\n", buf_recv);
			}
			usleep(200000);
		}
	}

	/* 关闭套接字 */
	close(client_sockfd);

	return 0;
}

