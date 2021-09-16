
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

static unsigned char rgb24[PIC_SIZE];
static unsigned char bgr24_c[PIC_SIZE];
static unsigned char bgr24_asm[PIC_SIZE];

void my_rgb24_bgr24_test(void)
{
	int i;

	for(i = 0; i < PIC_SIZE; i ++)
		rgb24[i] = i;

	for(i = 0; i < (PIC_SIZE / 3); i++){
		bgr24_c[3 * i] = rgb24[3 * i + 2];
		bgr24_c[3 * i + 1] = rgb24[3 * i + 1];
		bgr24_c[3 * i + 2] = rgb24[3 * i];
	}

	neon_rgb24_bgr24_test(rgb24, bgr24_asm, PIC_SIZE);
	for(i = 0; i < PIC_SIZE; i++){
		if(bgr24_c[i] != bgr24_asm[i]){
			printk("neon_rgb24_bgr24_test err\n");
			while(1);
		}
	}

}





