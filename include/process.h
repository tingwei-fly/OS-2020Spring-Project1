#include <sys/types.h>

#define MAX_POLICY_NAME 10
#define MAX_PROCESS_NAME 40

typedef struct process{
	char name[MAX_PROCESS_NAME];
	int readyTime;
	int execTime;
	pid_t pid;
}Process;

int execProcess(Process* process);
