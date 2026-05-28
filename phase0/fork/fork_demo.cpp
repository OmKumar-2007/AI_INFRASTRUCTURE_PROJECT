#include<stdio.h> 
#include<unistd.h>
#include<sys/wait.h>

int main(){
	printf("Before fork: PID=%d\n", getpid());

	int value = 43;

	pid_t pid = fork();


	if(pid<0){
		perror("fork failed");

		return 1;
	}

	if(pid == 0){

		printf("Child:  PID=%d, PPID=%d\n", getpid(), getppid());

		value = 100;

		printf("Child:  value=%d\n", value);

	}else {

		printf("Parent: PID=%d, child PID=%d\n", getpid(), pid);

		wait(NULL);

		printf("Parent: value=%d (unchanged)\n", value);
        	// Still 42 — the child's modification didn't affect parent's memory
    	}

	return 0;

}


