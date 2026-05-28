#include<stdio.h>
#include<fcntl.h>
#include<unistd.h>
#include <cstdlib>

int main(){
	printf("Before opening anything:\n");
	int fd3 = open("/etc/passwd", O_RDONLY);
	printf("Opened /etc/passwd, got fd: %d\n", fd3);

	int fd4 = open("/etc/hostname", O_RDONLY);
	printf("Opened /etc/hostname, got fd: %d\n", fd4);

	char cmd[64];
	sprintf(cmd, "ls -la /proc/%d/fd", getpid());

	system(cmd);

	close(fd3);
	close(fd4);

	 printf("\nAfter closing:\n");
	 system(cmd);
	 return 0;

}
