#include <sys/types.h>

#define MAIN_CPU 0
#define CHILD_CPU 1

#define PRINTK 334
#define GET_TIME 335

#define WAKEUP_PRIORITY 99
#define SUSPEND_PRIORITY 1

#define UNIT_T() for(volatile long long i = 0 ; i < 1000000UL ; i++);


void runSchedule(Process* process, char* policy, int N);
int compare(const void* a, const void* b);
void assignCPU(pid_t pid, int coreID);
void setProcessPriority(pid_t pid, int priority);
void selectNextProcess(Process* process, char* policy, int N, int startScheduler, pid_t* runningProcess, int* runningTime);
