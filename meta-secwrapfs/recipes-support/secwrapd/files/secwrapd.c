#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

void* daemon_listener(void *arg){
	printf("secwwrapd: Background listener thread spawned successfully\n");
       	return NULL;
}

int main(){
	pthread_t thread_id;
	printf("SecWrapFS Daemon Starting\n");
	if(pthread_create(&thread_id, NULL, daemon_listener, NULL) != 0){
		printf("Failed to Created thread\n");
	}
	pthread_join(thread_id, NULL);
	printf("secwrapd: Exiting\n");
	return 0;			
}
	
