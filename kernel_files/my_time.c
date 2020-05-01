// syscall 335

#include <linux/ktime.h>
#include <linux/timekeeping.h>
#include <linux/kernel.h>

asmlinkage void sys_my_time(unsigned long long *sec, unsigned long long *nsec){
	struct timespec t;
	getnstimeofday(&t);
	*sec = t.tv_sec;
	*nsec = t.tv_nsec;
	return;
}
