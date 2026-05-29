#include<sys/socket.h>
#include<netinet/in.h>
#include<unistd.h>
#include<cstring>
#include<cstdio>

int create_server(int port){
	int server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(server_fd<0) { perror("socket");return -1;}

	int opt=1;
	setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(port);

	if(bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) <0 ){
		perror("bind");
		close(server_fd);
		return -1;
	}

	if(listen(server_fd, 128) < 0) {
		perror("listen");
		close(server_fd);
		return -1;
	}
	printf("Server ready on port %d (fd = %d)\n" , port, server_fd);
	return server_fd;
}

int main(){
	int port = 8080;
	create_server(port);
	return 0;
}

