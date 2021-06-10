***************************************************************
;
;  PUNMSG.ASM
;  	Punch a message onto paper tape in human readable
;	form. Since version 2.0, this program supports a
;	punch on either port of an 88-2SIO or on an 88-SIO
;	board at 6/7 (e.g., the ACR address). Support of
;	automatic punch on/off has also been added in v2.0
;
;	Written by Mike Douglas using paper tape font data
;	and code from Martin Eberhard.
;
;	Rev	 Date	  Desc
;	1.0	10/3/13  Original
;
;	1.1	12/15/18 Fix code that did not wait for the
;		      Transmit Buffer Empty signal before
;		      outputting the blank vertical line
;		      between characters. This caused the
;		      last column of the character to be
;		      lost depending on the UART.
;
;	1.2	8/24/19 Fix the Rev 1.1 fix which tested the
;		      wrong polarity of the TDRE status bit.
;
;	2.0	2/16/20 Substantial re-write. Support multiple
;		      ports and options, including automatic
;		      punch on/off in a single version.
;       2.1     6/16/21 Added SIO 0/1
;
;**************************************************************
BDOS	equ	5		;Jump vector to BDOS
CONDIR	equ	6		;BDOS console direct I/O
WRTLINE	equ	9		;BDOS write a line to the console
RDLINE	equ	10		;BDOS read a line from the console

IBUFLEN	equ	100		;length of input buffer
CR	equ	0Dh		;ASCII carriage return
LF	equ	0Ah		;ASCII line feed
DC2	equ	12h		;punch on
DC4	equ	14h		;punch off

; 2SIO Equates and ACR equates

SIO1SR	equ	10h		;status register port 1
SIO1DR	equ	11h		;data register port 1
SIO2SR	equ	12h		;status register port 2
SIO2DR	equ	13h		;data register port 2

SIOTDRE	equ	02h		;bit for transmit ready
SIORDRF	equ	01h		;bit for receive data present

ACRSR	equ	06h		;status register port 2
ACRDR	equ	07h		;data register port 2

ACRTDRE	equ	80h		;bit for transmit ready
ACRRDRF	equ	01h		;bit for receive data present	

SIOSR	equ	00h		;status register
SIODR	equ	01h		;data register

;----------------------------------------------------------------------
;  Program loads and enters at 0100h
;----------------------------------------------------------------------
	org	0100h		;CP/M expects programs at 0100h

	mvi	a,3		;initialize 2nd 2SIO port
	out	SIO2SR
	mvi	a,11h		;8N2
	out	SIO2SR

; Get port to use from the operator

	lxi	d,mPort		;DE->port message
	call	askUser		;display and get response
	rz			;exit to CP/M if nothing typed

	sui	'1'		;'1' to binary 0
	cpi	4		;0,1,2,3 will generate carry
	rnc			;exit to CP/M for invalid port

	sta	portNum		;save punch port selection

; See if we flip characters (readable from bottom of tape

	lxi	d,mFlip		;DE->render on bottom message
	call	askUser		;display and get response
	rz			;exit to CP/M if nothing typed

	ori	20h		;'N' to 'n'
	sui	'n'		;A=0 if manual punch
	sta	fFlip		;save flip character flag

; See if we should use auto punch-on, punch-off (DC2, DC4)

	lxi	d,mAuto		;DE->automatic punch option
	call	askUser		;display and get response
	rz			;exit to CP/M if nothing typed

	ori	20h		;'N' to 'n'
	sui	'n'		;A=0 if manual punch
	sta	fAuto		;save auto punch flag

; Get phrase to punch and punch it. If the console is the punch and
;    is not an automatic punch, then prompt and wait for the user to
;    turn on the punch.

getMsg	lxi	d,mPhrase	;DE->phrase to punch message
	call	askUser
	rz			;exit to CP/M if nothing typed

	lda	fAuto		;automatic punch on/off?
	ora	a
	jz	notAuto		;no

	mvi	a,DC2		;DC2=punch on
	call	punByte		;send it
	jmp	punRdy		;continue with message

notAuto	lxi	d,mPunch	;DE->punch instructions
	mvi	c,WRTLINE
	call	BDOS		;display punch instructions
	call	waitCon		;wait for console input
	
punRdy	mvi	b,17		;17 leading blanks 
	call	blanks	
	
	call	punMsg		;punch the message

	mvi	b,30		;30 trailing blanks 
	call	blanks	

	lda	fAuto		;automatic punch on/off?
	ora	a
	jz	waitUsr		;no, wait for user to press CR

	mvi	a,DC4		;DC4=punch off
	call	punByte		;send it
	jmp	getMsg		;do it again

waitUsr	call	waitCon		;wait for console input
	jmp	getMsg		;do it again

;----------------------------------------------------------------------
; askUser - Display message passed in DE, return single character
;    response in A. If nothing typed, return status is zero. Full
;    response is at inbuf+2 with length at inbuf+1.
;----------------------------------------------------------------------
askUser	mvi	c,WRTLINE	;display the message passed in DE
	call	BDOS

	lxi	d,inbuf		;DE->input buffer
	mvi	a,IBUFLEN	;set max size to read
	stax	d
	mvi	c,RDLINE	;C=BDOS read line command
	call	BDOS		;read the user's response

	lda	inbuf+1		;how many bytes read?
	ora	a
	rz			;exit to CP/M if nothing typed

	lda	inbuf+2		;return first byte of the response
	ret

;----------------------------------------------------------------------
; punMsg - Punch message from inbuf onto tape in human
;    readable form. String may only contain characters
;    between 20h and 61h inclusive
;----------------------------------------------------------------------
punMsg	lda	inbuf+1		;A=character count
	mov	c,a		;C=character count
	lxi	h,inbuf+2	;HL->message to print
	
punLoop	mov	a,m		;A=next character
	push	h		;save message pointer
	
	ani	7Fh		;get rid of MSBit
	cpi	FMAX+1		;lower case character?
	jc	punUc		;no

	ani	5Fh		;'a'-'z' to 'A' to 'Z'

punUc	sui	FMIN		;subtract starting offset

	mvi	d,0		;compute HL = FONT + A*FSIZE
	mov	e,a		;DE=char-FMIN
	lxi	h,FONT		;HL->FONT table
	mvi	b,FSIZE		;B=columns per character

fontP0	dad	d		;multiply by B
	dcr	b
	jnz	fontP0
	
	mvi	b,FSIZE		;B=max columns per character

fontP1	mov	a,m		;get one column of the character
	cpi	SKP		;SKP means done for skinny chars
	jz	fontP2

	call	flip		;flip top/bottom if needed
	call	punByte		;punch the column

	ani	7Fh		;clear MSBit of data just punched
	cpi	DC4		;did we just turn the punch off?
	jnz	punOk		;no

	mvi	a,DC2		;turn the punch back on
	call	punByte	

punOk	inx	h		;move to next column
	dcr	b		;decrement column count
	jnz	fontP1

fontP2	xra	a		;punch blank columnt
	call	punByte

	pop	h		;restore HL message pointer
	inx	h		;point to next character

	dcr	c		;decrement character count
	jnz	punLoop

	ret

;----------------------------------------------------------------------
; blanks - Punch the number of blanks specified in B
;----------------------------------------------------------------------
blanks	xra	a		;A=blank to punch
	
blankLp	call	punByte		;punch a blank
	dcr	b
	jnz	blankLp

	ret

;----------------------------------------------------------------------
; flip - If fFlip is set, then flip bits so that font is
;    flipped top to bottom. This makes the font read properly
;    from the back of the tape where there are not pre-printed
;    messages. Clobbers DE.
;----------------------------------------------------------------------
flip	mov	d,a		;save the input byte in D
	lda	fFlip		;are we flipping?
	ora	a	
	mov	a,d		;restore the input byte
	rz			;no

	push	b		;preserve B and C
	mvi	b,8		;flipping 8 bits

fLoop	mov	a,d		;rotate input byte left
	ral
	mov	d,a		;D=rotated input byte

	mov	a,e		;rotate output byte right
	rar		
	mov	e,a		;E=rotated output byte

	dcr	b		;do 8 bits
	jnz	fLoop

	mov	a,e		;return result in A
	pop	b		;restore BC
	ret

;----------------------------------------------------------------------
; punByte - Punch the byte from A through the pseudo-port specified
;    by portNum.
;----------------------------------------------------------------------
punByte	push	psw		;save the byte to punch
	
	lda	portNum		;A=pseudo port number
	dcr	a
	jm	pSio1		;1st port on 2SIO

	jz	pSio2		;2nd port on 2SIO

        dcr     a
        jz      pAcr

; Use SIO port

pSio    in      SIOSR
        rlc
        jc      pSio
        pop     psw
        out     SIODR
        ret

; Use ACR port

pAcr	in	ACRSR		;wait for ACR port to be ready
	rlc			;MSBit is transmit ready
	jc	pAcr		;negative logic

	pop	psw		;A=byte to punch
	out	ACRDR
	ret

; Use 1st port on 2SIO

pSio1	in	SIO1SR		;wait for port to be ready
	ani	SIOTDRE
	jz	pSio1

	pop	psw		;A=byte to punch
	out	SIO1DR
	ret

; Use 2nd port on 2SIO

pSio2	in	SIO2SR		;wait for port to be ready
	ani	SIOTDRE
	jz	pSio2

	pop	psw		;A=byte to punch
	out	SIO2DR
	ret

;----------------------------------------------------------------------
; waitCon - Wait for user to type something and then exit
;----------------------------------------------------------------------
waitCon	mvi	e,0FFh		;FFh means input
	mvi	c,CONDIR	;console direct I/O
	call	BDOS

	ora	a		;anything entered?
	jz	waitCon		;no, keep waiting

	ret

;----------------------------------------------------------------------
; Data Area
;----------------------------------------------------------------------
mPort	db	CR,LF,'PUNMSG v2.1'
	db	CR,LF,LF,'Choose Port:'
	db	CR,LF,'  1) 2SIO Port #1'
	db	CR,LF,'  2) 2SIO Port #2'
	db	CR,LF,'  3) ACR Port'
	db	CR,LF,'  4) SIO Port'
	db	CR,LF,'Selection (x to exit): $'

mFlip	db	CR,LF,LF,'Render on bottom of tape (y/n)? $'
mAuto	db	CR,LF,'Use DC2/DC4 for punch on/off (y/n)? $'
mPhrase	db	CR,LF,LF,'Enter phrase to punch (RETURN to exit): $'
mPunch	db	CR,LF,LF,'Turn on the punch, then press RETURN.',CR,LF
	db	'(When complete, turn off the punch and press RETURN) $'

portNum	ds	1		;punch pseudo port number (0,1,2)
fAuto	ds	1		;non-zero if automatic punch
fFlip	ds	1		;non-zero if font should be flipped

inbuf	ds	IBUFLEN+1	;1st byte is read length

;----------------------------------------------------------------------
; 5x8 font for printing banner on tape. The LSBit is the bottom.
;----------------------------------------------------------------------
FMIN	EQU	20H		;minimum supported character
FMAX	EQU	60H		;maximum supported character
FSIZE	EQU	7		;columns per character
SKP	EQU	0FDH		;skip further columns

FONT:	DB	0,0,0,0,0,0,0			;20H SPACE
	DB	0,5Fh,0,SKP,0,0,0		;21H EXCLAIMATION PT
	DB	7,0,7,SKP,0,0,0			;22H DOUBLE QUOTE
	DB	28H,28H,0FEH,28H,0FEH,28H,28H	;23H #
	DB	48H,54H,54H,0FEH,54H,54H,24H	;24H $
	DB	8EH,4AH,2EH,10H,0E8H,0A4H,0E2H	;25H %
	DB	76H,89H,89H,95H,62H,0A0H,SKP	;26H &
	DB	0,6,0,SKP,0,0,0			;27H SINGLE QUOTE
	DB	3CH,42H,81H,SKP,0,0,0		;28H (
	DB	81H,42H,3CH,SKP,0,0,0		;29H (
	DB	24H,18H,7EH,18H,24H,SKP,0	;2AH *
	DB	10H,10H,7CH,10H,10H,SKP,0	;2BH +
	DB	40H,30H,SKP,0,0,0,0		;2CH COMMA
	DB	10H,10H,10H,10H,10H,SKP,0	;2DH MINUS
	DB	0,80H,0,SKP,0,0,0		;2EH PERIOD
	DB	2,4,8,10H,20H,40H,80H		;2FH /
	DB	7CH,86H,8AH,92H,0A2H,0C2H,7CH	;30H 0
	DB	84H,82H,0FEH,80H,80H,SKP,0	;1
	DB	84H,0C2H,0A2H,92H,8CH,SKP,0	;2
	DB	44H,92H,92H,92H,92H,6CH,SKP	;3
	DB	1EH,10H,10H,10H,0FEH,10H,SKP	;4
	DB	5EH,92H,92H,92H,92H,62H,SKP	;5
	DB	7CH,92H,92H,92H,92H,64H,SKP	;6
	DB	82H,42H,22H,12H,0AH,6,SKP	;7
	DB	6CH,92H,92H,92H,92H,6CH,SKP	;8
	DB	0CH,92H,92H,92H,92H,6CH,SKP	;39H 9
	DB	0,28H,0,SKP,0,0,0		;3AH COLON
	DB	80H,68H,SKP,0,0,0,0		;3BH SEMICOLON
	DB	10H,28H,44H,82H,SKP,0,0		;3CH <
	DB	28H,28H,28H,28H,28H,SKP,0	;3DH =
	DB	82H,44H,28H,10H,SKP,0,0		;3EH >
	DB	6,1,0A1H,11H,0EH,SKP,0		;3FH ?
	DB	7CH,82H,0BAH,0AAH,0BAH,0A2H,1CH	;40H @
	DB	0F8H,24H,22H,22H,24H,0F8H,SKP	;41H A
	DB	0FEH,92H,92H,92H,92H,6CH,SKP	;B
	DB	7CH,82H,82H,82H,82H,44H,SKP	;C
	DB	0FEH,82h,82H,82H,44H,38H,SKP	;D
	DB	0FEH,92H,92H,92H,82H,82H,SKP	;E
	DB	0FEH,12H,12H,12H,2,2,SKP	;F
	DB	38H,44H,82H,0A2H,0A2H,64H,SKP	;G
	DB	0FEH,10H,10H,10H,10H,0FEH,SKP	;H
	DB	82H,82H,0FEH,82H,82H,SKP,0	;I
	DB	40H,80H,80H,82H,7EH,2,SKP	;J
	DB	0FEH,20H,10H,28H,44H,82H,SKP	;K
	DB	0FEH,80H,80H,80H,80H,80H,SKP	;L
	DB	0FEH,4,8,10H,8,4,0FEH		;M
	DB	0FEH,4,8,10H,20H,0FEH,SKP	;N
	DB	7CH,82H,82H,82H,82H,7CH,SKP	;O
	DB	0FEH,12H,12H,12H,12H,0CH,SKP	;P
	DB	38H,44H,82H,92H,0A2H,44H,0B8H	;Q
	DB	0FEH,12H,12H,32H,52H,8CH,SKP	;R
	DB	4CH,92H,92H,92H,92H,64H,SKP	;S
	DB	2,2,2,0FEH,2,2,2		;T
	DB	7EH,80H,80H,80H,80H,7EH,SKP	;U
	DB	6,18H,60H,80H,60H,18H,6		;V
	DB	3EH,40H,80H,60H,80H,40H,3EH	;W
	DB	82H,44H,28H,10H,28H,44H,82H	;X
	DB	2,4,8,0F0H,8,4,2		;Y
	DB	82H,0C2H,0A2H,92H,8AH,86H,82H	;5AH Z
	DB	0FFH,81H,81H,SKP,0,0,0		;5BH L BRACKET
	DB	80H,40H,20H,10H,8,4,2		;5CH \
	DB	81H,81H,0FFH,SKP,0,0,0		;5DH R BRACKET
	DB	8,4,2,4,8,SKP,0			;5EH CARROT
	DB	80H,80H,80H,80H,80H,80H,80H	;5FH UNDERLINE
	DB	0,0FFH,0,SKP			;60H |

	end
                       