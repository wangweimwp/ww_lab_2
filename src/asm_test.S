

.global strings
strings:
	.string "wangwei test"

.align 3
.global my_data
my_data:
	.word 0x44

#define MY_LABLE  0x30

.align 3
.global adrp_data
adrp_data:
	.dword 0x12345678abcdabcd


.global ldr_str_test
ldr_str_test:
	mov x0, #0x123
	ldr x1, =0x80000
	str x0, [x1]

	ldr x0, strings
	ldr x1, =strings

	ldr x2, my_data
	ldr x3, =my_data

	ldr x4, MY_LABLE
	ldr x5, =MY_LABLE
	
	ret


.global adrp_ldr_test
adrp_ldr_test:
	adr x0, adrp_data

	adrp x1, adrp_data
	add x1, x1, #:lo12:adrp_data

	ldr x2, =adrp_data

	adrp x3, init_pg_dir
	ldr x4, =init_pg_dir

	ret


.global __memset_16bytes
__memset_16bytes:
	mov x4, #0
1:
	stp x1, x1, [x0], #16
	add x4, x4, #16
	cmp x4, x2
	b.ls 1b

	ret



.section .data
.align 3
.global my_atomic_data
my_atomic_data:
	.dword 0x0

.section .text
.global atomic_set
atomic_set:
1:
	ldxr x2, [x1]
	orr x2, x2, x0
	stxr w3, x2, [x1]
	cbnz w3, 1b
	ret 

.global my_atomic_write
my_atomic_write:
	adr x6, my_atomic_data
1:
	ldxr x2, [x6]
	orr x2, x2, x0
	stxr w3, x2, [x6]
	cbnz w3, 1b

	mov x0, x2
	ret
	

