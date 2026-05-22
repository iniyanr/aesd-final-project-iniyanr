#include <stdio.h>

int main(int argc, char *argv[]){
	printf("SecWrapFS Control Utility Skeleton Running.\n");
	if(argc>1){
		printf("Command Argument Received: %s.\n",argv[1]);
	}else{
		printf("Usage : secwrapctl [command]\n");
	}
	return 0;
}
