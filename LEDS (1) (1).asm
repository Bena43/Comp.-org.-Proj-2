	add $t0, $zero, $zero 		# t0 = 0 <----  ,  t0=1 -------->, a flag that saves the direction of movement
	add $t2, $zero, $zero		# t2 = 0, BTNC counter of last iteration
	res $imm, $imm, $imm ,1		# IORegister[1] = 1
	res $zero, $imm, $imm ,2	# IORegister[2] = 0
WHILE:
	res $t1, $zero, $zero 		# t1 = IORegister[0]
WHILE2:
	res $t3, $zero, $zero		# t3 = IORegister[0]
	sub $t3, $t3, $t1		# t3 = t3 - t1
	sub $t3, $t3, $imm, 56		# 56 is a random number we check in order to make the light to change every 5 sec
	ble $imm, $t3, $zero,WHILE2	# if (t3 > 0), which means, if we passed enough clock time, need to go to next light
	res $t3, $imm, $zero,2 		# t3 = IORegister[2]
	sub $t3, $t3, $t2		# t3 = t3 - t2
	res $t2, $imm, $zero,2 		# t2 = IORegister[2]
	add $t3, $t3, $t0		# t3 = t3 + flag
	and $t0, $t3, $imm, 1		# flag = t3 & 1
					# Now we check if we're in the edges
	res $t3, $imm, $zero,1   	# t3 = IORegister[1]
	add $t3, $t3, $imm, -1		# t3 = t3 - 1				
	bne $imm, $t3, $zero,NOT1	# if (t3 = 0), set flag = 0, else, dont change the flag
	add $t0, $zero, $zero		# flag = 0
NOT1:
	res $t3, $imm, $zero, 1		# t3 = IORegister[1]
	add $t3, $t3, $imm,-128		# t3 = t3 - 128
	bne $imm, $t3,$zero, NOT128	# if (t3 = 0), set flag = 0, else, dont change the flag
	add $t0, $zero, $imm, 1		# flag = 1
NOT128:
	res $t3, $imm, $zero, 1		# t3 = IORegister[1]
	beq $imm, $zero, $t0, FLAG0
	sra $t3, $t3, $imm, 1		# t3 >>= 1
	beq $imm, $zero, $zero, FIN
FLAG0:
	sll $t3, $t3, $imm, 1		# t3 <<= 1
FIN:
	res $t3, $imm, $imm, 1		# IORegister[1] = t3
	beq $imm, zero, zero, WHILE	