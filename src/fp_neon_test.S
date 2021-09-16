


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


.global neon_test_ld2
neon_test_ld2:
	mov x5, xzr
1:
	ld2 {v0.16b, v1.16b}, [x0], 32
	st2 {v0.16b, v1.16b}, [x1], 32
	add x5, x5, 32
	cmp x5, x2
	blt 1b

	ret

.global neon_test_ld3
neon_test_ld3:
	mov x5, xzr
1：
	ld3 {v0.16b, v1.16b, v2.16b}, [x0], 48
	st3 {v0.16b, v1.16b, v2.16b}, [x1], 48
	add x5, x5, 48
	cmp x5, x2
	blt 1b

	ret
	

