


.global fp_test
fp_test:
	fmov s0, #5.25
	fmov d1, #5.25
	ret

.global neon_test
neon_test:
	mov x5, xzr
1:
	ld1 {v0.16b}, [x0], 16
	st1 {v0.16b}, [x1], 16
	add x5, x5, 16
	cmp x5, x2
	blt 1b

	ret

