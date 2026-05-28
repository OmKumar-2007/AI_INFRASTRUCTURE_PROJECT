#include <stdio.h>    // printf, perror
#include <unistd.h>   // fork(), execvp()
#include <sys/wait.h> // waitpid(), WEXITSTATUS(), WIFEXITED()


int main(){
	 printf("Parent (PID %d): about to fork\n", getpid());

	 pid_t pid = fork();

	 if(pid==0){
		printf("Child (PID %d): calling exec...\n", getpid());
		
		const char *args[] = {"ls", "-la", "/proc/self/fd", NULL};

		execvp("ls", (char * const*)args);

		perror("exec failed");
		return 1;
	}

	 int status;
	 waitpid(pid, &status, 0);

	 if(WIFEXITED(status)){
		 printf("Parent: child exited with status %d\n", WIFEXITED(status));
	 }

	 return 0;
}
