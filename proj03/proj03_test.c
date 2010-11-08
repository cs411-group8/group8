#include <linux/unistd.h>
#include <sys/syscall.h>
#include <stdio.h>

#define __NR_get_slob_amt_claimed 338
#define __NR_get_slob_amt_free 339


int main(int argc, char *argv[]) {
	printf("Slob_amt_claimed: %d\r\n", syscall(__NR_get_slob_amt_claimed));
	printf("Slob_amt_free: %d\r\n", syscall(__NR_get_slob_amt_free));
	
	return 0;
}

