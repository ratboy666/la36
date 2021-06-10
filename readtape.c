/* readtape.c
 *
 * Read a paper tape. PUNMSG.COM can produce a readable leader for
 * paper tape (punch the leader, and turn it sideways to read it).
 *
 * Paper tapes can be capture by la36, using ^ARfilename punch ^AR
 * PUNMSG can punch a readable leader, and readtape can display the
 * data as if it were on the tape. 9 (may 10) characters can be
 * displayed (80 columns wide)
 *
 * readtape <MSG
 *
 * *    * ****** *      *       ****
 * *    * *      *      *      *    *
 * *    * *      *      *      *    *
 * ****** ****   *      *      *    *
 * *    * *      *      *      *    *
 * *    * *      *      *      *    *
 * *    * ****** ****** ******  ****
 *
 * PUNMSG punches 17 leading 00 bytes, and 30 trailing 00 bytes, so
 * we skip leading 00, and also stop if 00 count >= 9
 *
 * MSG contains:
 *
 * 00000000  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
 * 00000010  00 fe 10 10 10 10 fe 00  fe 92 92 92 82 82 00 fe
 * 00000020  80 80 80 80 80 00 fe 80  80 80 80 80 00 7c 82 82
 * 00000030  82 82 7c 00 00 00 00 00  00 00 00 00 00 00 00 00
 * 00000040  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
 *
 * and is produced by capturing the output of PUNMSG.COM
 *
 * PUNMSG.COM has been modified to output to 2SIO A, 2SIO B, ACR and
 * SIO. SIO added because I direct TTY to SIO.
 */

#include <stdio.h>
#include <stdlib.h>

int main(int ac, char **av) {
    int i, c, z, n;
    char *strip1, *strip2, *strip3, *strip4;
    char *strip5, *strip6, *strip7, *strip8;

    n = 80;
    if (ac > 1) {
	if (av[1][0] == '-') {
	    fprintf(stderr, "readtape [n] <file\n");
	    fprintf(stderr, "   display printable leader on tape file\n");
	    fprintf(stderr, "   n is maximum display columns (must be >= 40)\n");
	    return 1;
	}
	n = atoi(av[1]);
    }
    if (n < 40) {
	fprintf(stderr, "n must be >= 40\n");
	return 1;
    }
    strip1 = malloc(n + 1); strip2 = malloc(n + 1);
    strip3 = malloc(n + 1); strip4 = malloc(n + 1);
    strip5 = malloc(n + 1); strip6 = malloc(n + 1);
    strip7 = malloc(n + 1); strip8 = malloc(n + 1);
    for (i = 0; i < n; ++i)
	strip1[i] = strip2[i] = strip3[i] = strip4[i] =
	strip5[i] = strip6[i] = strip7[i] = strip8[i] = ' ';
    while ((c = getchar()) == 0)
	;
    for (i = z = 0; (i < n) && (c != EOF) && (z < 10); ++i) {
	if (c & 0x01) strip1[i] = '*';
	if (c & 0x02) strip2[i] = '*';
	if (c & 0x04) strip3[i] = '*';
	if (c & 0x08) strip4[i] = '*';
	if (c & 0x10) strip5[i] = '*';
	if (c & 0x20) strip6[i] = '*';
	if (c & 0x40) strip7[i] = '*';
	if (c & 0x80) strip8[i] = '*';
	c = getchar();
	if (c == 0)
	    ++z;
	else
	    z = 0;
    }
    strip1[i] = strip2[i] = strip3[i] = strip4[i] =
    strip5[i] = strip6[i] = strip7[i] = strip8[i] = '\0';
    printf("%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n", strip1, strip2, strip3, strip4,
		                               strip5, strip6, strip7, strip8);
    return 0;
}

