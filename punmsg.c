/* punmsg.c
 *
 * from punmsg.asm
 */

#include <stdio.h>
#ifdef z80
#include <unixio.h>
#include <sys.h>
#else
#include <unistd.h>
#endif

/* 7 wide, 8 high 0x20..0x60 */

#define SKP 0xFD

unsigned char font[] = {
0,0,0,0,0,0,0,
0,0x5F,0,SKP,0,0,0,
7,0,7,SKP,0,0,0,
0x28,0x28,0xFE,0x28,0xFE,0x28,0x28,
0x48,0x54,0x54,0xFE,0x54,0x54,0x24,
0x8E,0x4A,0x2E,0x10,0xE8,0xA4,0xE2,
0x76,0x89,0x89,0x95,0x62,0xA0,SKP,
0,6,0,SKP,0,0,0,
0x3C,0x42,0x81,SKP,0,0,0,
0x81,0x42,0x3C,SKP,0,0,0,
0x24,0x18,0x7E,0x18,0x24,SKP,0,
0x10,0x10,0x7C,0x10,0x10,SKP,0,
0x40,0x30,SKP,0,0,0,0,
0x10,0x10,0x10,0x10,0x10,SKP,0,
0,0x80,0,SKP,0,0,0,
2,4,8,0x10,0x20,0x40,0x80,
0x7C,0x86,0x8A,0x92,0xA2,0xC2,0x7C,
0x84,0x82,0xFE,0x80,0x80,SKP,0,
0x84,0xC2,0xA2,0x92,0x8C,SKP,0,
0x44,0x92,0x92,0x92,0x92,0x6C,SKP,
0x1E,0x10,0x10,0x10,0xFE,0x10,SKP,
0x5E,0x92,0x92,0x92,0x92,0x62,SKP,
0x7C,0x92,0x92,0x92,0x92,0x64,SKP,
0x82,0x42,0x22,0x12,0xA,6,SKP,
0x6C,0x92,0x92,0x92,0x92,0x6C,SKP,
0xC,0x92,0x92,0x92,0x92,0x6C,SKP,
0,0x28,0,SKP,0,0,0,
0x80,0x68,SKP,0,0,0,0,
0x10,0x28,0x44,0x82,SKP,0,0,
0x28,0x28,0x28,0x28,0x28,SKP,0,
0x82,0x44,0x28,0x10,SKP,0,0,
6,1,0xA1,0x11,0xE,SKP,0,
0x7C,0x82,0xBA,0xAA,0xBA,0xA2,0x1C,
0xF8,0x24,0x22,0x22,0x24,0xF8,SKP,
0xFE,0x92,0x92,0x92,0x92,0x6C,SKP,
0x7C,0x82,0x82,0x82,0x82,0x44,SKP,
0xFE,0x82,0x82,0x82,0x44,0x38,SKP,
0xFE,0x92,0x92,0x92,0x82,0x82,SKP,
0xFE,0x12,0x12,0x12,2,2,SKP,
0x38,0x44,0x82,0xA2,0xA2,0x64,SKP,
0xFE,0x10,0x10,0x10,0x10,0xFE,SKP,
0x82,0x82,0xFE,0x82,0x82,SKP,0,
0x40,0x80,0x80,0x82,0x7E,2,SKP,
0xFE,0x20,0x10,0x28,0x44,0x82,SKP,
0xFE,0x80,0x80,0x80,0x80,0x80,SKP,
0xFE,4,8,0x10,8,4,0xFE,
0xFE,4,8,0x10,0x20,0xFE,SKP,
0x7C,0x82,0x82,0x82,0x82,0x7C,SKP,
0xFE,0x12,0x12,0x12,0x12,0xC,SKP,
0x38,0x44,0x82,0x92,0xA2,0x44,0xB8,
0xFE,0x12,0x12,0x32,0x52,0x8C,SKP,
0x4C,0x92,0x92,0x92,0x92,0x64,SKP,
2,2,2,0xFE,2,2,2,
0x7E,0x80,0x80,0x80,0x80,0x7E,SKP,
6,0x18,0x60,0x80,0x60,0x18,6,
0x3E,0x40,0x80,0x60,0x80,0x40,0x3E,
0x82,0x44,0x28,0x10,0x28,0x44,0x82,
2,4,8,0xF0,8,4,2,
0x82,0xC2,0xA2,0x92,0x8A,0x86,0x82,
0xFF,0x81,0x81,SKP,0,0,0,
0x80,0x40,0x20,0x10,8,4,2,
0x81,0x81,0xFF,SKP,0,0,0,
8,4,2,4,8,SKP,0,
0x80,0x80,0x80,0x80,0x80,0x80,0x80,
0,0xFF,0,SKP,0,0,0
};

int main(int ac, char **av) {
    int i;
    unsigned char c, *s, *f;

#ifdef z80
    av = _getargs((char *)0x81, "punmsg");
    ac = _argc_;
#endif

    if (ac != 2) {
	printf("punmsg \"MESSAGE\"");
	return 1;
    }
    for (i = 0; i < 17; ++i) {
	c = 0;
        write(1, &c, 1);
    }
    s = (unsigned char *)av[1];
    while (*s) {
        c = *s;
	if (c == '/')
	    c = '\\';
	else if (c == '\\')
	    c = '/';
	else if (c == '|')
	    c = '`';
	else if (c >= 0x60)
	    c = c - 0x20;
	if ((c >= 0x20) && (c <= 0x60)) {
	    f = font + ((c - 0x20) * 7);
	    for (i = 0; i < 7; ++i) {
	        c = *f++;
                if (c == SKP)
                    break;
                write(1, &c, 1);
	    }
	    c = 0;
            write(1, &c, 1);
	}
	++s;
    }
    for (i = 0; i < 30; ++i) {
	c = 0;
        write(1, &c, 1);
    }
    return 0;
}
