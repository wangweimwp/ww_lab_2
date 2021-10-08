.section .data

.align	3
print_hello_sve:
	.string "hello sve\n"

.section .text
.globl main
main:
	stp     x29, x30, [sp, -16]!

	mov x2, #2
	whilelo p0.s, xzr, x2
	mov z0.s, p0/z, #0x55

	mov x3, #5
	whilelo p1.b, xzr, x3
	mov z1.s, p0/z, #0x44
	mov z2.b, p0/z, #0x55
	cmple p2.s, p0/z, z1.s, z2.s
	cmple p3.b, p1/z, z1.b, z2.b

	adrp x0, print_hello_sve
	add x0, x0, :lo12:print_hello_sve
	bl printf

	mov x0, #0
	ldp  x29, x30, [sp], 16
	ret

