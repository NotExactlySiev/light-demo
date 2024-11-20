# malucart's soft float routines

.set noreorder
.set nomacro
.set noat

.set abs0, $a2
.set abs1, $a3
.set leadingzeros, $t0
.set signbit, $t1
.set sig1withouthiddenzero, $t2
.set sig0, $t4
.set sig1, $t5
.set hiddenone, $t6
.set exp0, $t7
.set exp1, $t8
.set inputsxor, $t9
.set sig, $v1
.set lzcs, $30
.set lzcr, $31

# operation: q20.12 -> float
# 4 cycles if a == 0
# 16 cycles if a != 0
.global qtof
qtof:
	beqz $a0, .Lret0 # 0 is a special case because the math below can't make an exponent of 0
	sra signbit, $a0, 31 # branch delay slot
	xor abs0, $a0, signbit
	subu abs0, abs0, signbit
	# above: abs0 = abs($a0), sign = sign($a0)
	mtc2 abs0, lzcs # count the zeros in abs0 to make the exponent
	sll signbit, signbit, 31 # reduce sign to just the MSB
	li exp0, 146
	mfc2 leadingzeros, lzcr
	sll sig, abs0, 1
	sllv sig, sig, leadingzeros
	srl sig, sig, 9 # make normalized significand while erasing MSB (becomes hidden bit)
	subu exp0, exp0, leadingzeros
	sll exp0, exp0, 23
	or $v0, signbit, exp0 # OR the 3 elements
	jr $ra
	or $v0, $v0, sig

# operation: float -> q20.12
# 7 cycles if abs(a) < (1/4096)
# 9 cycles if abs(a) >= (1/4096) && a is too large
# 15 cycles if abs(a) >= (1/4096) && a isn't too large
.global ftoq
ftoq:
	# extract exponent
	srl exp0, $a0, 23
	andi exp0, exp0, 0xFF
	addiu exp0, exp0, -115
	# return 0 early if input is 0 or too small
	bltz exp0, .Lret0
	# return INT_MIN if input is too large
	addiu $at, exp0, -30 # branch delay slot
	bgez $at, .Lretnan
	# extract significand
	lui hiddenone, 0x0080 # branch delay slot
	or sig, $a0, hiddenone
	sll sig, sig, 8
	li $at, 31
	subu $at, $at, exp0
	# turn into two's complement
	bgez $a0, .Lftoqpositive
	srl $v0, sig, $at # branch delay slot
	jr $ra
	neg $v0, $v0 # branch delay slot
.Lftoqpositive:
	jr $ra
	nop # branch delay slot
.Lretnan: # return's INT_MIN
	jr $ra
	lui $v0, 0x8000 # branch delay slot

.Lret0:
	jr $ra
	li $v0, 0 # branch delay slot
