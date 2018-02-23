    .text
	.globl main
    .data 
        size:.word  0x40000
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
	#read kernel

	#put parameter
	li $4, 0xa0800200  #addr
	li $5, 0x200  #0x200 is Kernel's offset in SSD
   #li %5, 0x60   #0x60  is program's offset in Kernel, so we don't use 0x60
    la $6,size
    lw $6,0($6)
	
    #use the function: read SSD ,write it to ram 
	jal 0x8007b1a8	#func addr
	#jr $31		#func return
    #jump to the first function in Kernel
	jal 0xa08002bc	#start addr of kernel
