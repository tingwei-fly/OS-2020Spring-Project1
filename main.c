#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "include/process.h"
#include "include/scheduler.h"

#define _GNU_SOURCE
#define MAX_POLICY_NAME 10
#define MAX_PROCESS_NAME 40

int main(int argc, char** argv){
	char policy[MAX_POLICY_NAME] = {};
	int N;

	assert(scanf("%s", policy)==1);
	if(strcmp(policy,"FIFO")==0 || strcmp(policy,"RR")==0 || strcmp(policy, "SJF")==0 || strcmp(policy,"PSJF")==0){
	}else{
		fprintf(stderr,"Invalid Policy Name %s\n",policy);
		exit(0);
	}
	
	assert(scanf("%d", &N)==1);

	Process* process = (Process*)malloc(sizeof(Process) * N);
	for(int i = 0 ; i < N ; i++){
		assert(scanf("%s %d %d", process[i].name, &process[i].readyTime, &process[i].execTime)==3);
	}
	runSchedule(process, policy, N);
	return 0;
}
