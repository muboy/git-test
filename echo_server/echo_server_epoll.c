#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BACKLOG 512
#define MAX_EVENTS 128
#define MAX_MESSAGE_LEN 2048

void error(char* msg){
	fprintf(stderr, "%s\n", msg);
	exit(1);
}

int main(int argc, char * argv[]){

	if(argc < 2){
		printf("Please give a port number: ./epoll_echo_server [port]\n");
        exit(0);
	}

	// 工作端口
	int port = atoi(argv[1]);
	struct sockaddr_in server_addr, client_addr;

	// 接收数据缓冲区
	char buffer[MAX_MESSAGE_LEN];
	memset(buffer, 0, sizeof(buffer));

	// 简历服务端 socket
	int sock_listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(sock_listen_fd < 0){
		error("Error creating socket..");
	}

	// 初始化 服务端 socket
	memset((char*)&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	server_addr.sin_addr.s_addr = INADDR_ANY;
	//server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

	// 绑定 socket 配置信息
	if(bind(sock_listen_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
		error("Error binding socket..");
	}

	// 开始监听socket
	if(listen(sock_listen_fd, BACKLOG) < 0 ){
		error("Error listening..");
	}

	printf("epoll echo server listening for connections on port: %d\n", port);

	// 创建 epoll 
	struct epoll_event ev, events[MAX_EVENTS];
	int epoll_fd;

	epoll_fd = epoll_create(MAX_EVENTS);
	if(epoll_fd < 0){
		error("Error creating epoll..");
	}

	ev.events = EPOLLIN;
	ev.data.fd = sock_listen_fd;

	// 将 socket 添加到 epoll
	if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock_listen_fd, &ev) == -1){
		error("Error adding new listeding socket to epoll..");
	}

	socklen_t client_len = sizeof(client_addr);
	int recv_bytes = 0;

	while(1){
		// 获取到 可以活跃读的 socket 数量
		int new_events_count = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
		//printf("active client %d\n", new_events_count);
		
		if(new_events_count == -1){
			error("Error in epoll_wait..");
		}

		for(int i=0;i<new_events_count;++i){
			//sock_listen_fd 活跃，说明有新的连接建立
			if(events[i].data.fd == sock_listen_fd){
				// 得到新建立连接的 socket fd
				int sock_conn_fd = accept4(sock_listen_fd, (struct sockaddr*)&client_addr, &client_len, SOCK_NONBLOCK);
				if(sock_conn_fd == -1){
					error("Error accepting new connection..");
				}

				printf("new connection %s:%d\n",inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

				// 将这个新连接的读事件 注册监听
				ev.events = EPOLLIN | EPOLLET;
				ev.data.fd = sock_conn_fd;
				if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock_conn_fd, &ev) == -1){
					error("Error adding new event to epoll..");
				}
			}
			// 已经连接的用户，并且收到数据，进行读入
			else if(events[i].events & EPOLLIN){
				int new_sock_fd = events[i].data.fd;
				recv_bytes = recv(new_sock_fd, buffer, MAX_MESSAGE_LEN, 0);
				
				struct sockaddr_in peer;
				socklen_t peer_len = sizeof(peer);
				getpeername(new_sock_fd, (struct sockaddr*)&peer, &peer_len);
				
				printf("%s:%d send data: %s\n",inet_ntoa(peer.sin_addr), ntohs(peer.sin_port), buffer);

				if(recv_bytes <= 0 ){
					epoll_ctl(epoll_fd, EPOLL_CTL_DEL, new_sock_fd, 0);
					shutdown(new_sock_fd, SHUT_RDWR);
					printf("client %s:%d has closed\n",inet_ntoa(peer.sin_addr), ntohs(peer.sin_port));
				}else{
					ev.events = EPOLLOUT | EPOLLET;
					ev.data.fd = new_sock_fd;
					epoll_ctl(epoll_fd, EPOLL_CTL_MOD, new_sock_fd, &ev);
				}
			}
			// 已经连接的用户，有写事件
			else if(events[i].events & EPOLLOUT){
				//printf("%d write read\n", events[i].data.fd);
				buffer[recv_bytes] = '\0';
				send(events[i].data.fd, buffer, recv_bytes, 0);
				ev.events = EPOLLIN | EPOLLET;
				ev.data.fd = events[i].data.fd;
				epoll_ctl(epoll_fd, EPOLL_CTL_MOD, events[i].data.fd, &ev);
			}
			else{
				printf("what is it?\n");
			}
		}
	}

	return 0;
}
