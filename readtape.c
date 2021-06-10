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
 * we skip leading 00, and also stop if 00 count >= 3
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

#define LL 80

int main(int ac, char **av) {
    int i, c, z;
    char strip1[LL + 1];
    char strip2[LL + 1];
    char strip3[LL + 1];
    char strip4[LL + 1];
    char strip5[LL + 1];
    char strip6[LL + 1];
    char strip7[LL + 1];
    char strip8[LL + 1];

    for (i = 0; i < LL; ++i)
	strip1[i] =
	strip2[i] =
	strip3[i] =
	strip4[i] =
	strip5[i] =
	strip6[i] =
	strip7[i] =
	strip8[i] = ' ';
    while ((c = getchar()) == 0)
	;
    for (i = z = 0; (i < LL) && (c != EOF) && (z < 3); ++i) {
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
    strip1[i] =
    strip2[i] =
    strip3[i] =
    strip4[i] =
    strip5[i] =
    strip6[i] =
    strip7[i] =
    strip8[i] = '\0';
    printf("%s\n", strip1);
    printf("%s\n", strip2);
    printf("%s\n", strip3);
    printf("%s\n", strip4);
    printf("%s\n", strip5);
    printf("%s\n", strip6);
    printf("%s\n", strip7);
    printf("%s\n", strip8);
    return 0;
}

