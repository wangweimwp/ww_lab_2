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
	for(index_m = 0; index_m < n; index_m ++){
		for(index_n = 0; index_n < m; index_n ++){
			printf("%f  ", M[index_n, index_m]);
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
			C[index_n * index_m] =0;
			for(index_k = 0; index_k < k; index_k ++){
				C[index_n * index_m] += A[index_n, index_k] * B[index_k, index_m];
			}
		}
	}
	
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

	matrix_rand(A, n * k);
	matrix_rand(B, k * m);
	matrix_init(C, n, m, 0);
	matrix_multiply_c(A, B, C, n, m, k);

	clock_gettime(CLOCK_REALTIME, &time_start);
	matrix_print(C, n, k);
	clock_gettime(CLOCK_REALTIME, &time_end);
	clocks_c = (time_end.tv_sec - time_start.tv_sec)*1000000 +
			(time_end.tv_nsec - time_start.tv_nsec)/1000;
	printf("c spent time :%ld us\n", clocks_c);

	
}



