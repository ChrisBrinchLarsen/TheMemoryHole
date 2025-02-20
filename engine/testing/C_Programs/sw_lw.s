        .globl _start
_start:
	lui     t0,131072
    addi    t1,x0,1
    sw      t1,0(t0)
    lw      t2,0(t0) # Should be 1
	addi	a7,x0,3
	ecall
