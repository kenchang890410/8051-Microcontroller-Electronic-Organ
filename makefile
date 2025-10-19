HW1_7:
	sdcc -c Keypad4x4.c
	sdcc eOrgan-108321022.c Keypad4x4.rel
	packihx eOrgan-108321022.ihx > eOrgan-108321022.hex
	del *.asm *.ihx *.lk *.lst *.map *.mem *.rel *.rst *.sym