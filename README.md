# la36
la36 - Printing terminal emulator

la36 -? for command line help

This is a crude hack, and is a rewrite of Egan Ford's greenbar 0.1
utility. Greenbar shows us that the terminal emulator itself can
be used to emulate the printing device with the aid of a small
amount of code for sound, and background. la36 is both a terminal
emulator, and a printer emulator. If called with "-d device", la36
is a terminal emulator. If the "-d device" option is not used, "-i file"
must be used. la36 is then only a data sink. This is "printer mode". In printer
mode there is no command menu available. All options must be
preset on the command line:

      mkfifo la36_in
      ./la36 -i la36_in-h -n &
      ls >la36_in

Note that the -h and -n options are especially useful in this mode.
-h is the hold option. -hold is sent to xterm, and the window persists
after the application is complete. -n is the nl to cr/nl translation
option, which is very useful when sending unix/linux data to la36.
la36 converts overstrikes into bold, underline and strike-through
attributes.

Note that if la36 finds that it is *not* running in xterm, it will
restart itself running under xterm, with the appropriate xterm options.
This behaviour is very convenient, but also means that stdin / stdout /
stderr are lost. This is the reason that printer mode uses a fifo.


Notes:

In terminal mode, ^A (or whatever trigger character is defined)
activates commands. ^AH will give help on the available commands.
Enter to leave help and return to terminal.

In terminal mode, there may be a speed imbalance. The emulator will
work at the cps setting, but the serial link may be faster. In that
case characters may queue up. A lot of characters. ^AZ will dump the
input queue.

^AB is a special command mode; standard in and out are set to the
serial device and then the command is run. This is useful if (for
example) rx or sx are desired.

^AR and ^AS are file transfer (read/punch).

Requirements:

  xterm  la36 will relaunch itself under xterm. This code is very
         reliant on certain xterm features, and, in general, will NOT
         work with other terminal emulators. Uses VT100 line drawing,
         font changing, status line updates, underline, strike-through,
         inverse and bold attributes. Color selection and color setting.
         Screen size of 1920x1080 is needed -- other sizes may need
         adjusting of the font sizes.

  sox    sox is used to supply a "/usr/bin/play" utility which will play
         the sound samples used for hardware emulation. Modern systems
         are SO FAST that launching "play" on each character doesn't
         have much of an effect. Tested with a 10 year old Thinkpad 440,
         and appears to be fine. Greenbar was specific to Mac OS/X;
         this is a bit more universal. Note that sound is suppressed if
         the speed is greater than 120 characters per second.

  fonts  la36 uses mnicmp, Monaco and SV Basic Manual as fonts.
         These fonts are supplied. Note that mnicmp is slightly
         problematic as the underscore and overstrike emulation does
         not match the font.

               Monaco: programming font (Apple)
               mnicmp: dot matrix font (7-pin DECwriter II)
               SV Basic Manual: daisy wheel font (Diablo 630)

         The default font is Monaco. In terminal mode, ^A1, ^A2 and
         ^A3 select the font to be used.


Credits:

      greenbar, Version 0.1, written 2013 by Egan Ford (egan@sense.net)
      Z80pack, Udo Monk
      SV Basic Manual by Johan Winge
      mnicmp by Steward C. Russell
      Monaco by Susan Kare and Kris Holmes, Apple Inc.
      bell.wav, unknown

Egan Ford requests that you include credit to Egan Ford and greenbar
in any derivatives.

la36 is distributed under the same conditions as greenbar 0.1 because
it is a derivative work of greenbar 0.1.


Building:

      gcc -o la36 -Os la36.c
      strip la36


Motivations:

I aquired an Altair-Duino kit from Chris Davis. I used xterm as the
terminal because xterm supports full VT100 features. But, a Silent
700/703 or a real teletype was on my radar. Since I wanted the "look
and feel", I started writing my own terminal emulator. I then came
across greenbar 0.1 on Udo Monks Z80pack site. This did much of what
I wanted, but was not easy to interface to my Altair-Duino (being a
separate piece of hardware). I merged greenbar into my terminal
emulator, adding the fifo feature. I am still looking for an
inexpensive Silent 700/703, but this does the trick for now (and, no
wear and tear on a vintage printing terminal).


A Note on "shell":

shell is a script that uses socat:

  xport TERM=dumb
  sudo socat PTY,link=term,wait-slave EXEC:/bin/login,pty,setsid,setpgid,stderr,ctty &
  sleep 1
  sudo chmod 777 term
  ./la36 -d term -q -c 120
 
 socat copies data between two, um, things.  PTY creates a PTY (psuedo-terminal), that
 is a device that la36 can use (and SCREEN, minicom, picocom, etc). The other side
 is EXEC of the login program, which requests a login, and starts a terminal session.
 
 That can, of course, be any program! Or a socket.. (or other things). Without further ado:
 Using zxcc, my R program, and MBASIC.COM in /usr/local/lib/cpm/bin80/command.lbr
 
 mbasic contains
   zxcc r -MBASIC $@
   
then

  socat PTY,link=term,wait-slave EXEC:/home/fred/bin/mbasic &
  sleep 1
  sudo chmod 777 term
  la36 -d term -c 30

runs the Microsoft MBASIC interpreter under zxcc, as if on a 30 cps printing terminal.
l

 
 
 

