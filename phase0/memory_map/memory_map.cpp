#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>

int global_var = 42;

int uninitialized;

int main(){
	int stack_var = 10;

	int *heap_var = (int*)malloc(sizeof(int));

	*heap_var = 99;

	printf("PID: %d\n", getpid());

	printf("Code(main): %p\n", (void*)main);


	printf("Global:         %p\n", (void*)&global_var);

    	printf("Uninitialized:  %p\n", (void*)&uninitialized);

    	printf("Stack:          %p\n", (void*)&stack_var);

    	printf("Heap:           %p\n", (void*)heap_var);


	printf("\n--- Memory map from /proc ---\n");
    	char cmd[64];
    	sprintf(cmd, "cat /proc/%d/maps", getpid());
    	system(cmd);







	free(heap_var);
	return 0;

}
