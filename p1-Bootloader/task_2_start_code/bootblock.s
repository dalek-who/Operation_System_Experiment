	.text
	.globl main

main:
	# check the offset of main
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop

	#need add code
	# call printstr print a string

	li $8,0xbfe48000

	li $9,'H'
	sb $9,($8)

	li $9,'e'
	sb $9,($8)

	li $9,'l'
	sb $9,($8)

	li $9,'l'
	sb $9,($8)

	li $9,'o'
	sb $9,($8)

	
	li $9,'!'
	sb $9,($8)

	li $9,'w'
	sb $9,($8)


	li $9,'o'
	sb $9,($8)

	li $9,'r'
	sb $9,($8)

	li $9,'l'
	sb $9,($8)

	li $9,'d'
	sb $9,($8)

	li $9,'!'
	sb $9,($8)

	jr $31

