
#include "fp_neon_test.h"

static unsigned char a[MEMCPY_SIZE] = {0};
static unsigned char b[MEMCPY_SIZE] = {0};
static unsigned char c[MEMCPY_SIZE] = {0};
static unsigned char d[MEMCPY_SIZE] = {0};


void my_fp_neon_test(void)
{
	int i;
	for(i = 0; i < MEMCPY_SIZE; i++){
		a[i] = i;
	}

	neon_test(a, b, MEMCPY_SIZE);

	for(i = 0; i < MEMCPY_SIZE; i ++){
		if(a[i] != b[i]){
			printk("ld1 memcpy err\n");
			while(1);
		}
	}

	neon_test_ld2(a, c, MEMCPY_SIZE);
	for(i = 0; i < MEMCPY_SIZE; i++){
		if(a[i] != c[i]){
			printk("ld2 memcpy err\n");
			while(1);
		}
	}

	neon_test_ld3(a, d, MEMCPY_SIZE);
	for(i = 0; i < MEMCPY_SIZE; i ++){
		if(a[i] != d[i]){
		printk("ld2 memcpy err\n");
		while(1);
		}
	}
}








