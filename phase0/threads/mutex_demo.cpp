#include <stdio.h>
#include <pthread.h>

int counter = 0;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
// mutex starts in UNLOCKED state

void* safe_increment(void* arg) {
    for (int i = 0; i < 100000; i++) {
        pthread_mutex_lock(&lock);
        // ↑ if another thread holds the lock, THIS thread SLEEPS here
        // kernel removes this thread from the CPU run queue
        // wakes it when the lock is released
        
        counter++;   // now SAFE — only one thread here at a time
        
        pthread_mutex_unlock(&lock);
        // ↑ release lock — if any threads were sleeping waiting for it,
        // kernel wakes one of them up
    }
    return NULL;
}

int main() {
    pthread_t t1, t2;
    pthread_create(&t1, NULL, safe_increment, NULL);
    pthread_create(&t2, NULL, safe_increment, NULL);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    printf("Final counter: %d (always 200000 now)\n", counter);
    return 0;
}
