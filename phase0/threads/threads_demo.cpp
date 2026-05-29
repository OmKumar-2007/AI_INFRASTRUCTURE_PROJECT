
#include<stdio.h>
#include<pthread.h>
#include<unistd.h>
#include<stdlib.h>

volatile int counter = 0;

void* thread_function(void* arg){
	int thread_id = *(int*)arg;
	int local_var = 0;

	printf("Thread %d starting" , thread_id);

	for(int i = 0; i < 10000; i++){
		counter++;
		local_var++;
	}

	printf("Thread %d done, Local_var = %d, Counter = %d",thread_id, local_var, counter);
        return NULL;
}

int main(){
	pthread_t t1, t2;
	int id1 = 1, id2 = 2;

	pthread_create(&t1, NULL, thread_function, &id1);
	pthread_create(&t2, NULL, thread_function, &id2);

	pthread_join(t1, NULL);
	pthread_join(t2, NULL);

	printf("Final counter: %d", counter);
	return 0;
}
