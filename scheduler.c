#define _GNU_SOURCE
//#define ERR_EXIT(a) {perror(a); exit(1);}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sched.h>
#include <errno.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/sysinfo.h>
#include "include/process.h"
#include "include/scheduler.h"

int schedulerStart;
int runningProcess;
int finishedProcessNum;
int runningTime;
pid_t defaultRunner;
pid_t nextProcess;

//// Scheduling Main Function ////

void runSchedule(Process* process, char* policy, int N){
	qsort(process, N, sizeof(Process), compare);

	assignCPU(getpid(),0);
	setProcessPriority(getpid(), WAKEUP_PRIORITY);

	defaultRunner = fork();
	if(defaultRunner < 0){
		fprintf(stderr, "[ERROR]	Fork error %d\n",errno);
		exit(0);
	}else if(defaultRunner == 0){ // defaultRunner
		while(1);
	}else if(defaultRunner > 0){
		assignCPU(defaultRunner,1);
		setProcessPriority(defaultRunner, WAKEUP_PRIORITY);
	}

	for(int i = 0 ; i < N ; i++){
		process[i].pid = -1;
	}

	schedulerStart = 0;
	runningProcess = -1;
	finishedProcessNum = 0;
	nextProcess = -1;
	
	while(1){
		if(runningProcess != -1 && process[runningProcess].execTime == 0){
			waitpid(process[runningProcess].pid, NULL, 0);
			setProcessPriority(defaultRunner, WAKEUP_PRIORITY);
			finishedProcessNum++;
			if(finishedProcessNum == N){
				kill(defaultRunner, SIGKILL);
				break;
			}
			nextProcess = (runningProcess + 1) % N;
			process[runningProcess].pid = -1;
			while(process[nextProcess].pid == -1 || process[nextProcess].execTime == 0){
				if(nextProcess == runningProcess) break;
				nextProcess = (nextProcess + 1) % N;
			}
			if(nextProcess == runningProcess)
				nextProcess = -1;
			runningProcess = -1;
		}
		
		for(int i = 0 ; i < N ; i++){
			if(process[i].readyTime == schedulerStart){
				process[i].pid = execProcess(&process[i]);
				printf("%s %d\n", process[i].name, process[i].pid);
				fflush(stdout);
				if(runningProcess == -1 && nextProcess == -1)
					nextProcess = i;
			}
			if(process[i].readyTime > schedulerStart)
				break;
		}

		selectNextProcess(process, policy, N , schedulerStart, &runningProcess, &runningTime);

		UNIT_T();

		if(runningProcess != -1){
			process[runningProcess].execTime -= 1;
		}
		schedulerStart++;
	}
	return;
}

//// Schedule Part ////

/* Comparison for sort process */
int compare(const void* a, const void* b){
	Process* p1 = (Process*)a;
	Process* p2 = (Process*)b;
	return p1->readyTime - p2->readyTime;
}

/* Assign Process a Specific CPU */
void assignCPU(pid_t pid, int coreID){
	if(coreID > get_nprocs()){
		fprintf(stderr, "[ERROR]	Invalid core index\n");
		exit(0);
	}
	cpu_set_t mask;
	CPU_ZERO(&mask);
	CPU_SET(coreID, &mask);

	if(sched_setaffinity(pid, sizeof(mask), &mask) != 0){
		fprintf(stderr, "[ERROR]	Set affinity error %d\n",errno);
		exit(0);
	}
	return;
}

/* Set new priority of the process to wake up or suspend it*/
void setProcessPriority(pid_t pid, int priority){
	struct sched_param p;
	if(priority > 50){
		p.sched_priority = priority;
		if(sched_setscheduler(pid, SCHED_FIFO, &p) < 0){
			fprintf(stderr, "[ERROR]	Set priority error %d\n",errno);
			exit(0);
		}
	}else{
		p.sched_priority = 0;
		if(sched_setscheduler(pid, SCHED_OTHER, &p) < 0){
			fprintf(stderr, "[ERROR]	Set priority error %d\n",errno);
			exit(0);
		}
	}
	return;
}

/* Choose next process to run by policy*/
void selectNextProcess(Process* process, char* policy, int N, int startScheduler, pid_t* runningProcess, int* runningTime ){
	int previousProcess = *runningProcess;
	// don't preempt FIFO or SJF running process
	if(*runningProcess != -1 &&  ( (strcmp(policy, "FIFO")==0) || (strcmp(policy, "SJF")==0))){ 
		return;
	}else if(strcmp(policy,"PSJF")==0 || strcmp(policy,"SJF")==0 ){
		for(int i = 0 ; i < N ; i++){
			if(process[i].pid == -1 || process[i].execTime == 0){
				continue;
			}
			// compare exec time of processes
			if(previousProcess == -1 || process[i].execTime < process[previousProcess].execTime){
				previousProcess = i;
			}
		}
	}else if(strcmp(policy, "FIFO") == 0){
		
		if(previousProcess != -1){
			fprintf(stderr, "[ERROR]	FIFO process running error");
			exit(0);
		}
		
		for(int i = 0 ; i < N ; i++){
			if(process[i].pid == -1 || process[i].execTime == 0){ continue;}
			else if(previousProcess == -1 || process[i].readyTime < process[previousProcess].readyTime) {previousProcess = i;}
		}
	}else if(strcmp(policy, "RR") == 0){
		if(previousProcess == -1){ 
			previousProcess = nextProcess;
		}
		else if((schedulerStart - *runningTime) % 500 == 0){
			do{
				previousProcess++;
				previousProcess %= N;
			}while(process[previousProcess].pid == -1 || process[previousProcess].execTime == 0);
		}
	}
	////////////////////////////////
	if(*runningProcess == previousProcess){
		return;
	}
	else{
		setProcessPriority(process[previousProcess].pid, WAKEUP_PRIORITY);
		if(*runningProcess != -1){
			setProcessPriority(process[*runningProcess].pid, SUSPEND_PRIORITY);
		}else{
			setProcessPriority(defaultRunner, SUSPEND_PRIORITY);
		}	
		*runningProcess = previousProcess;
		*runningTime = startScheduler;
		return;
	}
}

//// Process Part ////

/* Execute a ready process and wait*/
int execProcess(Process* process){ // input is one process
	// fork()
	pid_t pid = fork();
	if(pid < 0){
		fprintf(stderr, "[ERROR]	Fork error %d\n",errno);
		exit(0);
	}else if(pid == 0){ //child process
		unsigned long long startSec, startNSec, endSec, endNSec;
		//get start time
		syscall(GET_TIME, &startSec, &startNSec);
		//
		//setProcessPriority(getpid(),WAKEUP_PRIORITY);
		//
		setProcessPriority(getpid(),SUSPEND_PRIORITY);
		//
		int execTime = process->execTime;
		for(int i = 0 ; i < execTime ; i++){
			UNIT_T();
		}
		//get end time
		syscall(GET_TIME, &endSec, &endNSec);
		syscall(PRINTK, getpid(), startSec, startNSec, endSec, endNSec);
		exit(0);
	}else if(pid > 0){
		assignCPU(pid,0);
		//
		setProcessPriority(pid,WAKEUP_PRIORITY);
		//
		setProcessPriority(getpid(),SUSPEND_PRIORITY);
		assignCPU(pid,1);
		setProcessPriority(pid,SUSPEND_PRIORITY);
		return pid;
	}
}
