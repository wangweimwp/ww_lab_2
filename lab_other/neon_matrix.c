#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>

#include <arm_neon.h>

#define A_ROW		4
#define B_CLOUMN	4
#define A_CLOUMN_B_ROW		4
#define LOOP	1000


static void matrix_rand(float *M, unsigned int num)
{
	int i = 0;
	for(i = 0; i < num; i++){
		M[i] = (float) rand() / (float ) RAND_MAX;
	}
}

static void matrix_init(float *M, unsigned int row, unsigned int cloumn,
						int val)
{
	int i, j;
	for(i = 0; i < row; i ++){
		for(j = 0; j < cloumn; j++){
			M[i * cloumn + j] = val;
		}
	}
}

static void matrix_print(float *M, unsigned int n, unsigned int m)
{
	int index_n, index_m;
	for(index_n = 0; index_n < n; index_n ++){
		for(index_m = 0; index_m < m; index_m ++){
			printf("%f  ", M[index_n * n + index_m]);
		}
		printf("\n");
	}
}

static void matrix_multiply_c(float *A, float *B, float *C,
								unsigned int n, unsigned int m, unsigned int k)
{
	int index_n, index_m, index_k;
	for(index_n = 0; index_n < n; index_n ++){
		for(index_m = 0; index_m < m; index_m ++){
			C[index_n + index_m * n] = 0;
			for(index_k = 0; index_k < k; index_k ++){
				C[index_n + index_m * n] += A[index_n + index_k * n] * B[index_k + index_m * k];
			}
		}
	}
	
}

static void matrix_multiply_asm(float *A, float *B, float *D)
{
	asm volatile(
		"ld1 {v0.4s, v1.4s, v2.4s, v3.4s}, [%0]\n"
		"ld1 {v4.4s, v5.4s, v6.4s, v7.4s}, [%1]\n"
		
		"movi v8.4s, 0\n"
		"movi v9.4s, 0\n"
		"movi v10.4s, 0\n"
		"movi v11.4s, 0\n"

		"fmla v8.4s, v0.4s, v4.4s[0]\n"
		"fmla v9.4s, v0.4s, v5.4s[0]\n"
		"fmla v10.4s, v0.4s, v6.4s[0]\n"
		"fmla v11.4s, v0.4s, v7.4s[0]\n"

		"fmla v8.4s, v1.4s, v4.4s[1]\n"
		"fmla v9.4s, v1.4s, v5.4s[1]\n"
		"fmla v10.4s, v1.4s, v6.4s[1]\n"
		"fmla v11.4s, v1.4s, v7.4s[1]\n"

		"fmla v8.4s, v2.4s, v4.4s[2]\n"
		"fmla v9.4s, v2.4s, v5.4s[2]\n"
		"fmla v10.4s, v2.4s, v6.4s[2]\n"
		"fmla v11.4s, v2.4s, v7.4s[2]\n"

		"fmla v8.4s, v3.4s, v4.4s[3]\n"
		"fmla v9.4s, v3.4s, v5.4s[3]\n"
		"fmla v10.4s, v3.4s, v6.4s[3]\n"
		"fmla v11.4s, v3.4s, v7.4s[3]\n"

		"st1 {v8.4s, v9.4s, v10.4s, v11.4s}, [%2]\n"
		:
		:"r" (A), "r" (B), "r" (D)
		: "memory", "v0", "v1", "v2", "v3",
			"v4", "v5", "v6", "v7", "v8",
			"v9", "v10", "v11"
	);
}


int main()
{
	int i;
	unsigned int n = A_ROW;
	unsigned int m = B_CLOUMN;
	unsigned int k = A_CLOUMN_B_ROW;

	struct timespec time_start, time_end;
	unsigned long clocks_c, clocks_neon, clocks_asm;

	float A[A_ROW * A_CLOUMN_B_ROW] = {0};
	float B[A_CLOUMN_B_ROW * B_CLOUMN] = {0};
	float C[A_ROW * B_CLOUMN] = {0};
	float D[A_ROW * B_CLOUMN] = {0};

	matrix_rand(A, n * k);
	matrix_print(A, n, k);
	matrix_rand(B, k * m);
	matrix_print(B, k, m);
	matrix_init(C, n, m, 0);
	

	clock_gettime(CLOCK_REALTIME, &time_start);
	for (i = 0; i < LOOP; i++)
		matrix_multiply_c(A, B, C, n, m, k);
	clock_gettime(CLOCK_REALTIME, &time_end);
	clocks_c = (time_end.tv_sec - time_start.tv_sec)*1000000 +
			(time_end.tv_nsec - time_start.tv_nsec)/1000;
	printf("c spent time :%ld us\n", clocks_c);
	matrix_print(C, n, m);
	printf("\n");
	matrix_multiply_asm(A, B, D);
	matrix_print(D, n, m);
}



