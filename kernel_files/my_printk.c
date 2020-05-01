// syscall 334

#include <linux/linkage.h>
#include <linux/kernel.h>

asmlinkage void sys_my_printk(int pid, unsigned long long startSec, unsigned long long startNSec, unsigned long long endSec, unsigned long long endNSec){
	printk("[Project1] %d %llu.%09llu %llu.%09llu\n", pid, startSec, startNSec, endSec, endNSec);
	return;
}
