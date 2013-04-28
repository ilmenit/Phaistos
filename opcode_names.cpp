char *opcode_name[]={
	/* 0x00 */ "brk",
	/* 0x01 */ "ora (d,x)",
	/* 0x02 */ "kil",
	/* 0x03 */ "slo (d,x)",
	/* 0x04 */ "nop d",
	/* 0x05 */ "ora d",
	/* 0x06 */ "asl d",
	/* 0x07 */ "slo d",
	/* 0x08 */ "php",
	/* 0x09 */ "ora #i",
	/* 0x0a */ "asl",
	/* 0x0b */ "anc #i",
	/* 0x0c */ "nop a",
	/* 0x0d */ "ora a",
	/* 0x0e */ "asl a",
	/* 0x0f */ "slo a",
	/* 0x10 */ "bpl *+d",
	/* 0x11 */ "ora (d,x)",
	/* 0x12 */ "kil",
	/* 0x13 */ "slo (d),y",
	/* 0x14 */ "nop d,x",
	/* 0x15 */ "ora d,x",
	/* 0x16 */ "asl d,x",
	/* 0x17 */ "slo d,x",
	/* 0x18 */ "clc",
	/* 0x19 */ "ora a,y",
	/* 0x1a */ "nop",
	/* 0x1b */ "slo a,y",
	/* 0x1c */ "nop a,x",
	/* 0x1d */ "ora a,x",
	/* 0x1e */ "asl a,x",
	/* 0x1f */ "slo a,x",
	/* 0x20 */ "jsr a",
	/* 0x21 */ "and (d,x)",
	/* 0x22 */ "kil",
	/* 0x23 */ "rla (d,x)",
	/* 0x24 */ "bit d",
	/* 0x25 */ "and a",
	/* 0x26 */ "rol d",
	/* 0x27 */ "rla d",
	/* 0x28 */ "plp",
	/* 0x29 */ "and #i",
	/* 0x2a */ "rol",
	/* 0x2b */ "anc #i",
	/* 0x2c */ "bit a",
	/* 0x2d */ "and a",
	/* 0x2e */ "rol a",
	/* 0x2f */ "rla a",
	/* 0x30 */ "bmi *+d",
	/* 0x31 */ "and (d),y",
	/* 0x32 */ "kil",
	/* 0x33 */ "rla (d),y",
	/* 0x34 */ "nop d,x",
	/* 0x35 */ "and d,x",
	/* 0x36 */ "rol d,x",
	/* 0x37 */ "rla d,x",
	/* 0x38 */ "sec",
	/* 0x39 */ "and a,y",
	/* 0x3a */ "nop",
	/* 0x3b */ "rla a,y",
	/* 0x3c */ "nop a,x",
	/* 0x3d */ "and a,x",
	/* 0x3e */ "rol a,x",
	/* 0x3f */ "rla a,x",
	/* 0x40 */ "rti",
	/* 0x41 */ "eor (d,x)",
	/* 0x42 */ "kil",
	/* 0x43 */ "sre (d,x)",
	/* 0x44 */ "nop d",
	/* 0x45 */ "eor d",
	/* 0x46 */ "lsr d",
	/* 0x47 */ "sre d",
	/* 0x48 */ "pha",
	/* 0x49 */ "eor #i",
	/* 0x4a */ "lsr",
	/* 0x4b */ "alr #i",
	/* 0x4c */ "jmp a",
	/* 0x4d */ "eor a",
	/* 0x4e */ "lsr a",
	/* 0x4f */ "sre a",
	/* 0x50 */ "bvc *+d",
	/* 0x51 */ "eor (d),y",
	/* 0x52 */ "kil",
	/* 0x53 */ "sre (d),y",
	/* 0x54 */ "nop d,x",
	/* 0x55 */ "and d,x",
	/* 0x56 */ "rol d,x",
	/* 0x57 */ "rla d,x",
	/* 0x58 */ "sec",
	/* 0x59 */ "and a,y",
	/* 0x5a */ "nop",
	/* 0x5b */ "rla a,y",
	/* 0x5c */ "nop a,x",
	/* 0x5d */ "and a,x",
	/* 0x5e */ "rol a,x",
	/* 0x5f */ "rla a,x",
	/* 0x60 */ "rts",
	/* 0x61 */ "adc (d,x)",
	/* 0x62 */ "kil",
	/* 0x63 */ "rra (d,x)",
	/* 0x64 */ "nop d",
	/* 0x65 */ "adc d",
	/* 0x66 */ "ror d",
	/* 0x67 */ "rra d",
	/* 0x68 */ "pla",
	/* 0x69 */ "adc #i",
	/* 0x6a */ "ror",
	/* 0x6b */ "arr #i",
	/* 0x6c */ "jmp (a)",
	/* 0x6d */ "adc a",
	/* 0x6e */ "ror a",
	/* 0x6f */ "rra a",
	/* 0x70 */ "bvs *+d",
	/* 0x71 */ "adc (d),y",
	/* 0x72 */ "kil",
	/* 0x73 */ "rra (d),y",
	/* 0x74 */ "nop d,x",
	/* 0x75 */ "adc d,x",
	/* 0x76 */ "ror d,x",
	/* 0x77 */ "rra d,x",
	/* 0x78 */ "sei",
	/* 0x79 */ "adc a,y",
	/* 0x7a */ "nop",
	/* 0x7b */ "rra a,y",
	/* 0x7c */ "nop a,x",
	/* 0x7d */ "adc a,x",
	/* 0x7e */ "ror a,x",
	/* 0x7f */ "rra a,x",
	/* 0x80 */ "nop #i",
	/* 0x81 */ "sta (d,x)",
	/* 0x82 */ "nop #i",
	/* 0x83 */ "sax (d,x)",
	/* 0x84 */ "sty d",
	/* 0x85 */ "sta d",
	/* 0x86 */ "stx d",
	/* 0x87 */ "sax d",
	/* 0x88 */ "dey",
	/* 0x89 */ "nop #i",
	/* 0x8a */ "txa",
	/* 0x8b */ "xaa #i",
	/* 0x8c */ "sty a",
	/* 0x8d */ "sta a",
	/* 0x8e */ "stx a",
	/* 0x8f */ "sax a",
	/* 0x90 */ "bcc *+d",
	/* 0x91 */ "sta (d),y",
	/* 0x92 */ "kil",
	/* 0x93 */ "ahx (d),y",
	/* 0x94 */ "sty d,x",
	/* 0x95 */ "sta d,x",
	/* 0x96 */ "stx d,y",
	/* 0x97 */ "sax d,y",
	/* 0x98 */ "tya",
	/* 0x99 */ "sta a,y",
	/* 0x9a */ "txs",
	/* 0x9b */ "tas a,y",
	/* 0x9c */ "shy a,x",
	/* 0x9d */ "sta a,x",
	/* 0x9e */ "shx a,y",
	/* 0x9f */ "ahx a,y",
	/* 0xa0 */ "ldy #i",
	/* 0xa1 */ "lda (d,x)",
	/* 0xa2 */ "ldx #i",
	/* 0xa3 */ "lax (d,x)",
	/* 0xa4 */ "ldy d",
	/* 0xa5 */ "lda d",
	/* 0xa6 */ "ldx d",
	/* 0xa7 */ "lax d",
	/* 0xa8 */ "tay",
	/* 0xa9 */ "lda #i",
	/* 0xaa */ "tax",
	/* 0xab */ "lax #i",
	/* 0xac */ "ldy a",
	/* 0xad */ "lda a",
	/* 0xae */ "ldx a",
	/* 0xaf */ "lax a",
	/* 0xb0 */ "bcs *+d",
	/* 0xb1 */ "lda (d),y",
	/* 0xb2 */ "kil",
	/* 0xb3 */ "lax (d),y",
	/* 0xb4 */ "ldy d,x",
	/* 0xb5 */ "lda d,x",
	/* 0xb6 */ "ldx d,y",
	/* 0xb7 */ "lax d,y",
	/* 0xb8 */ "clv",
	/* 0xb9 */ "lda a,y",
	/* 0xba */ "tsx",
	/* 0xbb */ "las a,y",
	/* 0xbc */ "ldy a,x",
	/* 0xbd */ "lda a,x",
	/* 0xbe */ "ldx a,y",
	/* 0xbf */ "lax a,y",
	/* 0xc0 */ "cpy #i",
	/* 0xc1 */ "cmp (d,x)",
	/* 0xc2 */ "nop #i",
	/* 0xc3 */ "dcp (d,x)",
	/* 0xc4 */ "cpy d",
	/* 0xc5 */ "cmp d",
	/* 0xc6 */ "dec d",
	/* 0xc7 */ "dcp d",
	/* 0xc8 */ "iny",
	/* 0xc9 */ "cmp #i",
	/* 0xca */ "dex",
	/* 0xcb */ "axs #i",
	/* 0xcc */ "cpy a",
	/* 0xcd */ "cmp a",
	/* 0xce */ "dec a",
	/* 0xcf */ "dcp a",
	/* 0xd0 */ "bne *+d",
	/* 0xd1 */ "cmp (d),y",
	/* 0xd2 */ "kil",
	/* 0xd3 */ "dcp (d),y",
	/* 0xd4 */ "nop d,x",
	/* 0xd5 */ "cmp d,x",
	/* 0xd6 */ "dec d,x",
	/* 0xd7 */ "dcp d,x",
	/* 0xd8 */ "cld",
	/* 0xd9 */ "cmp a,y",
	/* 0xda */ "nop",
	/* 0xdb */ "dcp a,y",
	/* 0xdc */ "nop a,x",
	/* 0xdd */ "cmp a,x",
	/* 0xde */ "dec a,x",
	/* 0xdf */ "dcp a,x",
	/* 0xe0 */ "cpx #i",
	/* 0xe1 */ "sbc (d,x)",
	/* 0xe2 */ "nop #i",
	/* 0xe3 */ "isc (d,x)",
	/* 0xe4 */ "cpx d",
	/* 0xe5 */ "sbc d",
	/* 0xe6 */ "inc d",
	/* 0xe7 */ "isc d",
	/* 0xe8 */ "inx",
	/* 0xe9 */ "sbc #i",
	/* 0xea */ "nop",
	/* 0xeb */ "sbc #i",
	/* 0xec */ "cpx a",
	/* 0xed */ "sbc a",
	/* 0xee */ "inc a",
	/* 0xef */ "isc a",
	/* 0xf0 */ "beq *+d",
	/* 0xf1 */ "sbc (d),y",
	/* 0xf2 */ "kil",
	/* 0xf3 */ "isc (d),y",
	/* 0xf4 */ "nop d,x",
	/* 0xf5 */ "sbc d,x",
	/* 0xf6 */ "inc d,x",
	/* 0xf7 */ "isc d,x",
	/* 0xf8 */ "sed",
	/* 0xf9 */ "sbc a,y",
	/* 0xfa */ "nop",
	/* 0xfb */ "isc a,y",
	/* 0xfc */ "nop a,x",
	/* 0xfd */ "sbc a,x",
	/* 0xfe */ "inc a,x",
	/* 0xff */ "isc a,x",
};