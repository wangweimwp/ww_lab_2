
#include "fp_neon_test.h"

static unsigned char a[MEMCPY_SIZE] = {0};
static unsigned char b[MEMCPY_SIZE] = {0};


void my_fp_neon_test(void)
{
	int i;
	for(i = 0; i < MEMCPY_SIZE; i++){
		a[i] = i;
	}

	neon_test(a, b, MEMCPY_SIZE);

	for(i = 0; i < MEMCPY_SIZE; i ++){
		if(a[i] != b[i]){
			printk("memcpy err\n");
			while(1);
		}
	}
}








