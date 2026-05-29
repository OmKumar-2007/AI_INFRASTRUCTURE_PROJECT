#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/socket.h>
#include<netinet/in.h>

#define PORT 8080

int main(){
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	int opt = 0;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(PORT);

	bind(fd, (struct sockaddr*)&addr, sizeof(addr));
	listen(fd, 10);

	printf("Blocking server on port %d\n", PORT);
    	printf("Open two terminal tabs and connect with: nc localhost 8080\n");
    	printf("Notice: typing in tab 2 does nothing until tab 1 disconnects\n\n");

	while(1){
		int client_fd = accept(fd, NULL, NULL);
		 printf("[SERVING] client fd=%d — ALL OTHER CLIENTS IGNORED\n", client_fd);

		char buf[1024];

		while(1){
			ssize_t n = recv(client_fd, buf, sizeof(buf)-1, 0);
			if(n<=0) break;
			buf[n] = '\0';
			printf("[fd %d] says: %s", client_fd, buf);

			send(client_fd, buf, n, 0);
		}
		close(client_fd);
		printf("[DONE] fd=%d disconnected. NOW accepting next client.\n\n", client_fd);
	}
return 0;
}
