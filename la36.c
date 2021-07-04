/*
 *  la36.c
 *
 */

#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <sys/select.h>
#include <sys/time.h>

#define NOTHING

#define VERSION "0.90"
#define MAXC    132
#define SHARE   "/usr/local/share/la36/"

/* Display
 *
 * Uncomment one of the display sizes.
 */
// #define DISPLAY_1920x1080
#define DISPLAY_1600x900
//#define DISPLAY_1024x768

/* UTF-8 characters
 */
#define EM "\xE2\x80\x83"
#define EN "\xE2\x80\x82"
#define TH "\xE2\x80\x89"

//#define CASSETTE "🖭 "
// #define CASSETTE "📼"
// "▶■" "⏵⏹"

/* Choose your title line cassette image. This blinks, and
 * the two sequences should have the same width (especially
 * if you title bar is centered). I use the "message waiting"
 * symbol (looks like a looped tape). On my desktop font,
 * this is 1 em + 1 thin wide. -- change this per your
 * desire. There is no perfect, or even good way, of doing
 * this...
 *
 * Actually, we can use the "invisible" attribute
 */
#define CASSETTE "➿"
#define CASSETTE_WHITE EM TH

/* Default settings
 */
int            cps        = 30;
int            hold       = 0;
int            xforce     = 0;
int            upper      = 0;
int            quiet      = 0;
int            plen       = 0;
char           trigger    = 'A';
char          *device     = NULL;
char          *input      = NULL;
int            font       = 0;
int            lfonly     = 0;
int            echo       = 0;
int            asr        = 0;
int            strip      = 0;

/* Global variables
 */
int            speed      = 0;
int            color      = 0;
int            lines      = 0;
int            std_in     = STDIN_FILENO;
int            std_out    = STDOUT_FILENO;
int            dev_in     = -1;
int            dev_out    = -1;
int            col        = 0;
int            lbuf[MAXC] = { 0, };
int            lp         = 0;
FILE          *rcvtape    = NULL;
FILE          *sndtape    = NULL;
int            fifo       = 0;
int            rcv_e      = 1;
int            snd_e      = 0;
int            rcv_n      = 0;
int            snd_n      = 0;
int            rcv_f      = 0;
int            snd_f      = 0;
char          *rcvname    = NULL;
char          *sndname    = NULL;

char          *fnames[]   = { "Monaco", "Monaco", "mnicmp", "SV Basic Manual" };

struct winsize win;
struct termios olds;
struct termios oldt;
struct termios news;
struct termios newt;

/* Select font
 */
void selfont(void) {
#ifdef DISPLAY_1920x1080
#define DSIZE "1920x1080"
    switch (font) {
    case 1:
        printf("\033]50;Monaco:size=16\007");
	break;
    case 2:
	printf("\033]50;mnicmp:size=20\007");
	break;
    case 3:
	printf("\033]50;SV Basic Manual:size=16\007");
	break;
    }
#endif
#ifdef DISPLAY_1600x900
#define DSIZE "1600x900"
    switch (font) {
    case 1:
        printf("\033]50;Monaco:size=12\007");
	break;
    case 2:
	printf("\033]50;mnicmp:size=16\007");
	break;
    case 3:
	printf("\033]50;SV Basic Manual:size=12\007");
	break;
    }
#endif
#ifdef DISPLAY_1024x768
#define DSIZE "1024x768"
    switch (font) {
    case 1:
        printf("\033]50;Monaco:size=7\007");
	break;
    case 2:
	printf("\033]50;mnicmp:size=10\007");
	break;
    case 3:
	printf("\033]50;SV Basic Manual:size=7\007");
	break;
    }
#endif
}

/* Update xterm status bar
 */
void status(char *s, ...) {
    char buf[256];
    va_list args;

    va_start(args, s);
    vsprintf(buf, s, args);
    va_end(args);
    printf("\033]2;%s\007", buf);
}

/* "Base" status - this is just the application name
 */
void status0(void) {
    status("la36 " VERSION);
}

void cursoroff(void);

/* Read string from operator
 */
char *reads(char *prompt) {
    static char buf[256];

    cursoroff();
    printf("\r\n");
    printf("\033[0m\r\n");
    printf("\033[?25h\033[?12h%s", prompt);
    tcsetattr(std_in, TCSAFLUSH, &oldt);
    fgets(buf, sizeof buf, stdin);
    if (buf[strlen(buf) - 1] == '\n')
	buf[strlen(buf) - 1] = '\0';
    tcsetattr(std_in, TCSAFLUSH, &newt);
    printf("\033[?25l");
    return buf;
}

void margin(void);
void lf2(void);

void ends(void) {
    col = 0;
    margin();
    lf2();
    printf("\033[?25l");
}

/* Restart application under xterm
 */
void xterm(int argc, char **argv) {
    char cmd[256];
    int i;

    if (xforce || (getenv("XTERM_VERSION") == NULL)) {
        strcpy(cmd, "/usr/bin/xterm");
        strcat(cmd, " -ti 340 -sl 20480");
#ifdef DISPLAY_19290x1080
        strcat(cmd, " -fa Monaco -fs 16");
#endif
#ifdef DISPLAY_1600x900
        strcat(cmd, " -fa Monaco -fs 12");
#endif
#ifdef DISPLAY_1024x768
        strcat(cmd, " -fa Monaco -fs 7");
#endif
        strcat(cmd, " -xrm \"XTerm*deleteIsDEL: true\"");
        strcat(cmd, " -xrm \"XTerm*allowTitleOps: true\"");
        strcat(cmd, " -xrm \"XTerm*allowFontOps: true\"");
        strcat(cmd, " -xrm \"XTerm*allowWindowOps: true\"");
        if (hold)
            strcat(cmd, " -hold");
        strcat(cmd, " -e \"");
        for (i = 0; i < argc; ++i) {
            if (i)
                strcat(cmd, " ");
            if (strcmp(argv[i], "-x") == 0) /* do not copy -x through */
                NOTHING;
            else
                strcat(cmd, argv[i]);
        }
        strcat(cmd, "\"");
        system(cmd);
        exit(0);
    }
}

/* Give usage information
 */
void usage(void) {
    printf("la36 " VERSION "\n\n");
    fprintf(stderr, "la36 [-q] [-u] [-h] [-x] [-z trigger] [-c cps] "
		    "[-f len] [-1] [-2] [-3] [-n]\n");
    fprintf(stderr, "     [-e ] [-t] [-7] -d device | -i file \n\n");
    fprintf(stderr, "  -d device  serial device\n");
    fprintf(stderr, "  -i file    input file/fifo\n");
    fprintf(stderr, "  -q         quiet (no sound)\n");
    fprintf(stderr, "  -u         uppercase\n");
    fprintf(stderr, "  -h         hold after exit\n");
    fprintf(stderr, "  -x         force xterm relaunch\n");
    fprintf(stderr, "  -z char    set trigger (default %c)\n", trigger);
    fprintf(stderr, "  -c cps     set cps 10..4000 (default %d)\n", cps);
    fprintf(stderr, "  -f length  set form length\n");
    fprintf(stderr, "  -1         use font 1 (Monaco)\n");
    fprintf(stderr, "  -2         set form 2 (mnicmp)\n");
    fprintf(stderr, "  -3         set form 3 (SV Basic Manual)\n");
    fprintf(stderr, "  -n         translate \\n to \\r\\n\n");
    fprintf(stderr, "  -e         local echo\n");
    fprintf(stderr, "  -t         enable asr (^Q/^S, ^R/^T)\n");
    fprintf(stderr, "  -7         strip incoming bit 7\n");
    fprintf(stderr, "  -?         this help\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "You must choose one of -i or -d. If -i is used,"
		    " -h (hold) is very useful, as\n");
    fprintf(stderr, "is -n, because the data is probably unix (with");
    fprintf(stderr, " newlines only).\n");
    exit(1);
}

/* handle options
 */
void options(int argc, char **argv) {
    int ch;
    opterr = 1;
    while ((ch = getopt(argc, argv, "?7qen123uhxtd:z:c:f:i:")) != -1)
        switch (ch) {
	case '7':
	    strip = 1;
	    break;
	case 'q':
	    quiet = 1;
	    break;
	case 'e':
	    echo = 1;
	    break;
	case 'n':
	    lfonly = 1;
	    break;
	case '1':
	    font = 1;
	    break;
	case '2':
	    font = 2;
	    break;
	case '3':
	    font = 3;
	    break;
	case 'u':
	    upper = 1;
	    break;
        case 'h':
            hold = 1;
            break;
        case 'x':
            xforce = 1;
            break;
	case 't':
	    asr = 1;
	    break;
        case 'd':
            device = strdup(optarg);
            break;
	case 'z':
	    trigger = toupper(optarg[0]);
	    break;
	case 'c':
	    cps = atoi(optarg);
	    break;
	case 'f':
	    plen = atoi(optarg);
	    break;
	case 'i':
	    input = strdup(optarg);
	    break;
	case '?':
	default:
	    usage();
	    break;
    }
}

/* Set color to green, white (or cyan -- which is not yet used)
 */
void setcolor(int c) {
    if (c == 0)
        printf("\033[42m"); /* green */
    else if (c == 1)
	printf("\033[47m"); /* white */
    else
	printf("\033[46m"); /* cyan */
}

/* Display character, with attributes ("wide character")
 *
 * Exploiting UTF-8, we could combine these:
 * combining long solidus   ̸
 * combining tilde  ̃
 * combining cicumflex  ̂
 * combining dot below  ̣
 *
 * These probably will not work in most fonts -- need to "try and see"
 *
 * But, to prepare for the possible implementation:
 *
 * wc character codes:
 *   a bcd efgh 0 1234567
 *
 *   The character value is in bits 1..7, with 0 being ' '
 *   bit '0' is not used (may be one more attribute, or char set extension)
 *   h = bold
 *   g = underline
 *   f = strikethrough
 *   e = italic
 *   b..d = combiner
 *      000 none
 *      001 /
 *      010 ~
 *      011 ^
 *      100 .
 *   bit 'a' is not used.
 *
 */
void wc(int c) {
    printf(" \010");
    if (c & 0x100) /* bold */
        printf("\033[1m");
    if (c & 0x200) /* underline */
        printf("\033[4m");
    if (c & 0x400) /* strikethrough */
        printf("\033[9m");
    if (c & 0x800) /* italic */
	printf("\033[3m");
    printf("%c", c & 0x7f ? c & 0x7f : ' ');
    if (c & 0xf00) { /* normal */
        printf("\033[0m");
        setcolor(color);
    }
}

/* Merge ca (character with attributes) with ch (new character),
 * returning character with attributes. This handles conversion of
 * overstrike (using cr and/or bs) to xterm screen attributes
 * (bold, underline, strike-through).
 *
 * With the mnicmp font, the strike-through is messed up, but the
 * effect is nice (I like it). Using the Monoaco font is nicer,
 * but is not "true to the la36".
 */
int mc(int ca, int ch) {
    int a = ca & 0xf00;
    int c = 0;
    ca = ca & 0x7f;
    ch = ch & 0x7f;
    if (ca == 0) ca = ' ';
    if (ch == 0) ch = ' ';
    if (a & 0x200) {
        if (ca == '_') ca = ' ';
        if (ch == '_') ch = ' ';
    }
    if (a & 0x400) {
        if (ca == '-') ca = ' ';
        if (ch == '-') ch = ' ';
    }
    if (ca == ' ') { c = ch;             goto rtn; }
    if (ch == ' ') { c = ca;             goto rtn; }
    if (ca == ch ) { c = ca; a |= 0x100; goto rtn; }
    if (ca == '_') { c = ch; a |= 0x200; goto rtn; }
    if (ch == '_') { c = ca; a |= 0x200; goto rtn; }
    if (ca == '-') { c = ch; a |= 0x400; goto rtn; }
    if (ch == '-') { c = ca; a |= 0x400; goto rtn; }
    // c = ca;
    // a |= 0x100;
    c = ch;
    a |= 0x800;
rtn:
    return c | a;
}

/* Cursor off - cursor is inverse space
 */
void cursoroff(void) {
    if (fifo == 0)
        wc(lbuf[col]);
}

/* Cursor display
 */
void cursor(void) {
    if (fifo == 0) {
        printf("\033[7m");
        wc(lbuf[col]);
        printf("\033[0m");
        setcolor(color);
        printf("\010");
    }
}

/* Hard copy margin, just past left feed holes
 */
void margin(void) {
    printf("\r\033[7C");
}

/* Display paper - either feed hole or not. Uses VT100 line draw
 */
void paper(int hole) {
    if (hole == 1)
        printf(
            "\033(0x ` x                                                                                                                                        x ` x\033(B"
	    "\r");
    else if (hole == 0)
	printf(
            "\033(0x   x                                                                                                                                        x   x\033(B"
	    "\r");
    else /* hole == 2, page break */
	printf(
            "\033(0x   x qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq x   x\033(B"
	    "\r");

}

/* Play sound file. Don't play sound if over 120 cps.
 */
void play(char *soundfile) {
    char cmd[256];
    if (cps > 120)
	return;
    if (!quiet) {
        strcpy(cmd, "DISPLAY= /usr/bin/play -q ");
	strcat(cmd, SHARE);
	strcat(cmd, soundfile);
	strcat(cmd, " &");
	system(cmd);
    }
}

/* Carriage return, from 5 zones
 */
void cr(void) {
    cursoroff();
    if ((col > 0) && (col <= 20)) {
	play("dwcr20.aiff");
	usleep(speed);
    } else if ((col > 20) && (col <= 40)) {
	play("dwcr40.aiff");
	usleep(2 * speed);
    } else if ((col > 40) && (col <= 60)) {
	play("dwcr60.aiff");
	usleep(3 * speed);
    } else if ((col > 60) && (col <= 80)) {
	play("dwcr80.aiff");
	usleep(4 * speed);
    } else {
	play("dwcr.aiff");
	usleep(5 * speed);
    }
    col = 0;
    margin();
    cursor();
}

/* Audible alarm
 */
void bell(void) {
    play("bell.wav");
    usleep(speed);
}

/* Line feed. Broken up into sections because this is also used for
 * form feed. We also translate lf to cr/lf if needed.
 */
void lf3(void) {
    printf("\r\n");
    if (lines == 0)
        color = color == 0;
    setcolor(color);
    paper(lines == 1);
    margin();
    if (col > 0)
	printf("\033[%dC", col);
    if (++lines == 3)
	lines = 0;
}

void lf2(void) {
    int i;
    for (i = 0; i < MAXC; ++i)
	lbuf[i] = 0;
    lf3();
    ++lp;
    if (plen && (lp >= plen)) {
	lp = 0;
	printf("\r");
	paper(2);
	lf3();
    }
} 

void lf(void) {
    if (lfonly)
	cr();
    play("dwnl.aiff");
    cursoroff();
    lf2();
    cursor();
    usleep(speed);
}

/* Form feed
 */
void ff(void) {
    if (plen <= 0)
	return;
    lf();
    if (lp == 0)
	return;
    cursoroff();
    while (lp)
        lf2();
    cursor();
    usleep(2 * speed);
}

/* Print normal charactar. All of the audio was similar,
 * so I just made brackets of characters. Could have
 * c & 7 (0..7) if 7 then 0
 */
void p(char c) {
    if (c < 43)        play("dwchar0.aiff");
    else if (c < 56)   play("dwchar1.aiff");
    else if (c < 69)   play("dwchar2.aiff");
    else if (c < 82)   play("dwchar3.aiff");
    else if (c < 95)   play("dwchar4.aiff");
    else if (c < 108)  play("dwchar5.aiff");
    else               play("dwchar.aiff");
    /* cursoroff(); */
    lbuf[col] = mc(lbuf[col], c);
    wc(lbuf[col]);
    if (++col >= MAXC) {
	cr();
	lf();
    }
    cursor();
    usleep(speed);
}

/* Backspace
 */
void bs(void) {
    if (col > 0) {
	cursoroff();
	--col;
	margin();
        if (col > 0)
	    printf("\033[%dC", col);
	cursor();
	usleep(speed);
    }
}

/* Tab - tabs are every 8 columns. In future, this will probably
 * change, because the DECwriter had settable tabs.
 */
void tab(void) {
    do /* Thank you, Hector Peraza ! */
	p(' ');
    while (col & 7);
}

/* Print a character, interpret controls. In future, may try to
 * interpret ESC for SIXEL graphics.
 */
void print(char c) {
    /* MBASIC NEEDS THIS FOR FILES COMMAND! MBASIC sets bit 7 high
     * on the last characters of each extension with the FILES
     * command. On CP/M, bit 7 is usually stripped. We do NOT
     * display non-ascii, so the characters are then ignored.
     */
    if (strip)
        c &= 0x7F;
    if (c == '\f')                     ff();
    else if (c == '\r')                cr();
    else if (c == '\a')                bell();
    else if (c == '\b')                bs();
    else if (c == '\t')                tab();
    else if (c == '\n')                lf();
    else if ((c >= ' ') && (c <= '~')) p(c);
}

/* Dump queued characters. This is useful when we are working at a
 * slow emulated rate, but cannot get the dev speed down that low
 * (eg, 300/9600). Linux will then buffer -- a LOT. This allows
 * dumping the received queue.
 */
void dq(void) {
    struct timeval tv = { 0, 0 };
    fd_set fds;
    char c;
    int n;

    status("dumping queue");
    for (;;) {
        FD_ZERO(&fds);
        FD_SET(dev_in, &fds);
        select(dev_in + 1, &fds, NULL, NULL, &tv);
	if (FD_ISSET(dev_in, &fds)) {
	    n = read(dev_in, &c, 1);
	} else {
	    status0();
	    return;
	}
    }
}

/* Interactive help. ^AH provides a summary of commands, and also
 * details current settings.
 */
char help(void) {
    int n;
    char c;
    cursoroff();
    printf("\r\n\033[0m\r\nla36 " VERSION "\r\n");
    printf("\r\n");
    printf("-- general                       -- send/receive\r\n");
    printf("  %c  - send ^%c                     s  - %s\r\n",
		    tolower(trigger), trigger,
                    sndtape ? "stop send" : "send file");
    printf("  h  - help                        r  - %s\r\n",
                    rcvtape ? "stop receive" : "receive file");
    printf("  x  - exit                        p  - local receive echo (now %s)\r\n", rcv_e ? "on " : "off");
    printf("  q  - toggle sound (now %s)      o  - local send echo (now %s)\r\n",
        quiet ? "off" : "on ", snd_e ? "on " : "off");
    printf("  u  - toggle upper (now %s)      t  - automatic send/receive (now %s)\r\n", 
        upper ? "on " : "off", asr ? "on " : "off");
    printf("  c  - cps (now %4d)            -- shell\r\n", cps);
    printf(" ' ' - resume terminal             !  - shell\r\n");
    printf("-- local                           b  - execute with serial\r\n");
    printf("  f  - form length (now %d)\r\n", plen);
    printf("  j  - local linefeed\r\n");
    printf("  l  - local formfeed\r\n");
    printf("  1  - select programming font\r\n");
    printf("  2  - select dot matrix font\r\n");
    printf("  3  - select daisy wheel font\r\n");
    printf("  e  - local echo (now %s)\r\n", echo ? "on " : "off");
    printf("  z  - dump receive queue\r\n");
    printf("  7  - 7 bit/8 bit toggle (now %d)\n", strip ? 7 : 8);
    status("space to continue (or command)?");
    do {
	usleep(1000);
	n - read(0, &c, 1);
    } while (n == -1);
    if (c != ' ') {
        status0();
        col = 0;
        margin();
        lf2();
    }
    return c;
}

/* Execute a command in a shell -- with stdin and stdout directed
 * to the serial device. Used for running rx and sx...
 */
void bshell(void) {
    char *s;

    status("bshell (enter command) ?");
    s = reads("> ");
    printf("\033[?25h\033[?12h");
    tcsetattr(std_in, TCSANOW, &oldt);
    tcsetattr(std_in, TCSANOW, &olds);
    status0();
    if (s[0]) {
	int old0 = dup(std_in);
	int old1 = dup(std_out);
	dup2(dev_in, std_in);
	dup2(dev_out, std_out);
        system(s);
	dup2(old0, std_in);
	dup2(old1, std_out);
	close(old0);
	close(old1);
    }
    tcsetattr(std_in, TCSANOW, &newt);
    tcsetattr(std_in, TCSANOW, &news);
    ends();
}

/* Execute a command in a shell -- if just a return is given,
 * a copy of bash is run.
 */
void shell(void) {
    char *s;

    status("shell (enter command) ?");
    s = reads("! ");
    printf("\033[?25h\033[?12h");
    tcsetattr(std_in, TCSANOW, &oldt);
    if (s[0] == 0)
        strcpy(s, "/usr/bin/bash");
    status0();
    system(s);
    tcsetattr(std_in, TCSANOW, &newt);
    ends();
}

/* Interactive command, initiate with control-trigger
 */
int command(void) {
    int n;
    char c;
    char *s;

    /* Get the next key. We use a busy loop polling for the second
     * key.
     */
    status("command (h for help/menu) ?");
    do {
	usleep(1000);
	n - read(0, &c, 1);
    } while (n == -1);
    status0();

    /* Trigger followed by trigger will send a single trigger
     * to the system we are communicating with
     */
    if ((c == trigger) ||
        (toupper(c) == trigger) ||
	(c == (trigger - '@'))) {
	c = trigger - '@';
	write(dev_out, &c, 1);
        return 0;
    }

    /* Interpret the command character.
     */
top:
    switch (c) {
        case 'x': case 'X': case 'X' - '@':
            cr();
            lf();
            return 1;
        case 't': case 'T': case 'T' - '@':
	    asr ^= 1;
	    break;
        case 'q': case 'Q': case 'Q' - '@':
	    quiet ^= 1;
	    break;
        case 'u': case 'U': case 'U' - '@':
	    upper ^= 1;
	    break;
	case 'e': case 'E': case 'E' - '@':
	    echo ^= 1;
	    break;
	case 'j': case 'J': case 'J' - '@':
	    cr();
	    lf();
	    break;
	case 'l': case 'L': case 'L' - '@':
	    cr();
	    ff();
	    break;
        case 'h': case 'H': case 'H' - '@': case '?':
	    c = help();
	    goto top;
	case ' ':
	    break;
        case '1':
	    font = 1;
	    selfont();
	    break;
	case '2':
	    font = 2;
	    selfont();
	    break;
	case '3':
	    font = 3;
	    selfont();
	    break;
        case '7':
	    strip ^= 1;
	    break;
        case '!':
	    shell();
	    break;
	case 'b': case 'B': case 'B' - '@':
	    bshell();
	    break;
	case 'f': case 'F': case 'F' - '@':
            status("form (0=continuous) ?");
	    plen = atoi(reads("form: "));
	    if (plen < 0)
		plen = 0;
	    ends();
	    break;
	case 'c': case 'C': case 'C' - '@':
            status("cps (0=unlimited) ?");
	    cps = atoi(reads("cps: "));
    	    if (cps > 4000)
		cps = 4000;
    	    if (cps < 0)
		cps = 10;
	    if (cps == 0)
		speed = 0;
	    else
                speed = 1000000 / cps;
	    ends();
	    break;
	case 's': case 'S': case 'S' - '@':
	    if (sndtape) {
		fclose(sndtape);
		free(sndname);
		sndname = NULL;
		sndtape = NULL;
		snd_f = 0;
		if ((sndtape == NULL) && (rcvtape == NULL))
                    status0();
	    } else {
	        status("send ?");
	        s = reads("send: ");
		sndname = strdup(s);
		sndtape = fopen(s, "r");
		snd_n = 0;
		if (!asr)
		    snd_f = 1;
		else
		    snd_f = 0;
		ends();
	    }
	    break;
	case 'r': case 'R': case 'R' - '@':
	    if (rcvtape) {
		fclose(rcvtape);
		free(rcvname);
		rcvname = NULL;
		rcvtape = NULL;
		rcv_f = 0;
		if ((sndtape == NULL) && (rcvtape == NULL))
                    status0();
	    } else {
	        status("receive ?");
	        s = reads("receive: ");
		rcvname = strdup(s);
		rcvtape = fopen(s, "w");
		rcv_n = 0;
		if (!asr)
		    rcv_f = 1;
		else
		    rcv_f = 0;
		ends();
	    }
	    break;
	case 'p': case 'P': case 'P' - '@':
	    rcv_e ^= 1;
	    break;
	case 'o': case 'O': case 'O' - '@':
	    snd_e ^= 1;
	    break;
	case 'z': case 'Z': case 'Z' - '@':
	    dq();
	    break;
    }
    status0();
    return 0;
}

/* Print out a string
 */
void printstring(char *s) {
    while (*s)
	print(*s++);
}

/* Animated tick for status line. We are called two times
 * a second.
 */
void tick(void) {
    static int t = 0;
    char *s;

    if (rcvtape || sndtape) {
        t ^= 1;
	s = t ? CASSETTE : CASSETTE_WHITE;
	if ((snd_f | rcv_f) == 0)
	    s = CASSETTE_WHITE;
	status("la36%s " VERSION " r:%s %06d / s:%s %06d %s",
	       asr ? "/asr" : "",
	       rcvname ? rcvname : "", rcv_n,
	       sndname ? sndname : "", snd_n, s);
    }
}

/* Handle asr functions. Done by complicated relay logic in the
 * ASR-33.
 */
int handle_asr(int c) {
    if (asr == 0)
	return 0;
    if (rcvtape && (c == ('Q' - '@'))) { /* start reader */
	rcv_f = 1;
    } else if (rcvtape && (c == ('S' - '@'))) { /* stop reader */
	rcv_f = 0;
    } else if (sndtape && (c == ('R' - '@'))) { /* start punch */
	snd_f = 1;
    } else if (sndtape && (c == ('T' - '@'))) { /* stop punch */
	snd_f = 0;
    } else
        return 0;
    return 1;
}


/* Terminal loop, transfer data from operator terminal
 * to serial and serial to terminal.
 */
void terminal(void) {
    struct timeval tv;
    struct timeval last = { 0, 0 };
    fd_set fds;
    int n;
    char c;

    col = 0;
    lp = 0;
    lines = 0;
    color = 0;
    lf();

    if (fifo) {

        /* Printer mode. Read from fifo la36_in, and write to
	 * terminal. On read fail, we assume end of input, and
	 * exit.
	 */

        for (;;) {
	    n = read(std_in, &c, 1);
	    if (n <= 0)
		return;
	    if (n == 1) {
	        if (upper)
	            c = toupper(c);
	        print(c);
	    }
	}
    }

    status0();

    for (;;) {

        /* Terminal mode. Look at both std_in and dev_in and
	 * copy to the other (std_in to dev_out and dev_in to
	 * std_out).
	 */

        FD_ZERO(&fds);
        FD_SET(std_in, &fds);
        FD_SET(dev_in, &fds);
        tv.tv_sec = 0;
        tv.tv_usec = speed;
	if (tv.tv_usec < 250) /* 38400 max send rate */
	    tv.tv_usec = 250;
	select(dev_in + 1, &fds, NULL, NULL, &tv);
	gettimeofday(&tv, NULL);
	if ((last.tv_sec != tv.tv_sec) ||
	    ((tv.tv_usec - last.tv_usec) > 500000)) {
	    tick();
	    last = tv;
        }
	if (FD_ISSET(std_in, &fds)) {
	    n = read(std_in, &c, 1);
	    if (n == 1) {
		if (c == (trigger - '@')) {
		    if (command())
			return;
		} else if (handle_asr(c)) {
	            ;
		} else {
		    if (upper)
		        c = toupper(c);
		    if (echo)
			print(c);
                    write(dev_out, &c, 1);
		}
	    }
	} else if (FD_ISSET(dev_in, &fds)) {
	    n = read(dev_in, &c, 1);
	    if (n == 1) {
                if (handle_asr(c)) {
		    ;
		} else if (rcvtape && rcv_f) {
		    ++rcv_n;
		    fputc(c, rcvtape);
		}
		if (upper)
		    c = toupper(c);
		if (rcvtape && rcv_f) {
		    if (rcv_e)
		        print(c);
		} else
		    print(c);
	    }
	} else {
	    if (sndtape && snd_f) {
		n = fgetc(sndtape);
		if (n == EOF) {
		    fclose(sndtape);
		    sndtape = NULL;
		    free(sndname);
		    sndname = NULL;
		} else {
		    c = n;
		    if (snd_e)
			print(c);
		    c = n;
		    write(dev_out, &c, 1);
		}
	    }
	}
    }
}

/* Signon giving option feedback
 */
void signon(void) {
    printf("la36 " VERSION " - %s mode\n\n",
	fifo == 0 ? "terminal" : "printer");
    if (fifo == 0) {
        printf("Hard copy terminal emulator with "
	       "Automatic send/receive (ASR)\n\n");
        printf("  Trigger ^%c\n", trigger);
        printf("  ^%cH for help\n", trigger);
        printf("  ASR is %s\n", asr ? "on" : "off");
    } else
        printf("Hard copy printer emulator\n\n");
    printf("  %d cps\n", cps);
    printf("  Sound %s\n", quiet ? "off" : "on");
    printf("  Uppercase %s\n", upper ? "on" : "off");
    printf("  Font %s\n", fnames[font]);
    printf("  LF to CR/LF translation %s\n",
	lfonly ? "enabled" : "disabled");
    printf("  Hold is %s\n", hold ? "enabled" : "disabled");
    if (plen > 0)
        printf("  Form length %d lines\n", plen);
    else
        printf("  Form continuous\n");
    printf("  Data bits %d\n", strip ? 7 : 8);
    printf("  la36 compiled for " DSIZE " pixel screen\n");
    printf("\n");
}

/* la36 main
 */
int main(int argc, char **argv) {

    options(argc, argv);

    if ((device == NULL) && (input == NULL)) {
	fprintf(stderr, "la36 " VERSION "\n");
	fprintf(stderr, "Either -d device must be used, or -i file\n");
	fprintf(stderr, "la36 does not use stdin\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "la36 -? for help\n");
	exit(1);
    }

    xterm(argc, argv);

    if (cps > 4000)
	cps = 4000;
    if (cps < 0)
	cps = 10;
    if (cps == 0)
	speed = 0;
    else
        speed = 1000000 / cps;
    if (plen < 0)
	plen = 0;

    signon();
    if (device) {
        dev_out = open(device, O_RDWR);
	if (dev_out < 0) {
	    fprintf(stderr, "can\'t open device %s\n", device);
	    exit(1);
	}
	dev_in = dev_out;
    } else if (input) {
	fifo = 1;
        printf("  Waiting for file/fifo...\n");
	std_in = open(input, O_RDONLY);
	if (std_in < 0) {
	    fprintf(stderr, "can\'t open %s for reading\n", input);
	    exit(1);
	}
	printf("  ...Connected\n");
    }

    ioctl(std_out, TIOCGWINSZ, &win);

    printf(
        "\033[?25l"
        "\033[8;;146t"
        "\033]4;2;#D6E8D3\007"
        "\033]4;7;#ECECDD\007"
	"\033[%d;0H", win.ws_row - 1);

    selfont();

    if (fifo == 0) {
        tcgetattr(dev_in, &olds);
        news = olds;
        cfmakeraw(&news);
        tcsetattr(dev_in, TCSANOW, &news);
        tcgetattr(std_in, &oldt);
        newt = oldt;
        cfmakeraw(&newt);
        tcsetattr(std_in, TCSANOW, &newt);
    }

    setvbuf(stdout, NULL, _IONBF, 0);

    terminal();

    if (fifo == 0) {
        tcsetattr(std_in, TCSANOW, &oldt);
        tcsetattr(dev_in, TCSANOW, &olds);
    }

    if (!hold) {
        printf(
            "\n\n"
            "\033[!p"
            "\033[?3;4l"
            "\033[4l"
            "\033>");
    }

    if (fifo == 0) {
        close(dev_in);
        close(dev_out);
    }
    return 0;
}
