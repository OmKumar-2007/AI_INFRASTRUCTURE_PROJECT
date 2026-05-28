#include<unistd.h>

int main(){
	syscall(1, 1, "Hello\n", 6);

	syscall(60, 0);

	return 0;
}

