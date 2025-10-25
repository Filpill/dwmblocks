#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<signal.h>
#ifndef NO_X
#include<X11/Xlib.h>
#endif
#ifdef __OpenBSD__
#define SIGPLUS			SIGUSR1+1
#define SIGMINUS		SIGUSR1-1
#else
#define SIGPLUS			SIGRTMIN
#define SIGMINUS		SIGRTMIN
#endif
#define LENGTH(X)               (sizeof(X) / sizeof (X[0]))
#define CMDLENGTH		50
#define MIN( a, b ) ( ( a < b) ? a : b )
#define STATUSLENGTH (LENGTH(blocks) * CMDLENGTH + 1)

typedef struct {
	char* icon;
	char* command;
	unsigned int interval;
	unsigned int signal;
	char* clickcmd;
} Block;
#ifndef __OpenBSD__
void dummysighandler(int num);
#endif
void sighandler(int num);
void sighandler_info(int signum, siginfo_t *si, void *ucontext);
void getcmds(int time);
void getsigcmds(unsigned int signal);
void setupsignals();
void sighandler(int signum);
int getstatus(char *str, char *last);
void statusloop();
void termhandler(int signum);
void pstdout();
#ifndef NO_X
void setroot();
static void (*writestatus) () = setroot;
static int setupX();
static Display *dpy;
static int screen;
static Window root;
#else
static void (*writestatus) () = pstdout;
#endif


#include "blocks.h"

static char statusbar[LENGTH(blocks)][CMDLENGTH] = {0};
static char statusstr[2][STATUSLENGTH];
static int statusContinue = 1;
static int returnStatus = 0;
static int button = 0;  // Store button click information

//opens process *cmd and stores output in *output
void getcmd(const Block *block, char *output)
{
	// Insert signal number as control character at the beginning
	if (block->signal) {
		output[0] = block->signal;
		output++;
	}

	int j = 0;

	strcpy(output+j, block->icon);
	FILE *cmdf = popen(block->command, "r");
	if (!cmdf)
		return;
	int i = j + strlen(block->icon);
	fgets(output+i, CMDLENGTH-i-delimLen-2, cmdf);
	i = strlen(output);
	if (i == j) {
		//return if block and command output are both empty
		pclose(cmdf);
		return;
	}
	//only chop off newline if one is present at the end
	i = output[i-1] == '\n' ? i-1 : i;

	if (delim[0] != '\0') {
		strncpy(output+i, delim, delimLen);
	}
	else
		output[i++] = '\0';
	pclose(cmdf);
}

void getcmds(int time)
{
	const Block* current;
	for (unsigned int i = 0; i < LENGTH(blocks); i++) {
		current = blocks + i;
		if ((current->interval != 0 && time % current->interval == 0) || time == -1)
			getcmd(current,statusbar[i]);
	}
}

void getsigcmds(unsigned int signal)
{
	const Block *current;
	for (unsigned int i = 0; i < LENGTH(blocks); i++) {
		current = blocks + i;
		if (current->signal == signal)
			getcmd(current,statusbar[i]);
	}
}

void setupsignals()
{
#ifndef __OpenBSD__
	/* initialize all real time signals with dummy handler */
	for (int i = SIGRTMIN; i <= SIGRTMAX; i++)
		signal(i, dummysighandler);

	/* Set up SIGUSR1 handler for dwm click signals */
	struct sigaction sa;
	sa.sa_sigaction = sighandler_info;
	sa.sa_flags = SA_SIGINFO;
	sigemptyset(&sa.sa_mask);
	sigaction(SIGUSR1, &sa, NULL);

	/* Set up real-time signal handlers for block updates */
	for (unsigned int i = 0; i < LENGTH(blocks); i++) {
		if (blocks[i].signal > 0)
			signal(SIGMINUS+blocks[i].signal, sighandler);
	}
#else
	/* OpenBSD setup */
	for (unsigned int i = 0; i < LENGTH(blocks); i++) {
		if (blocks[i].signal > 0)
			signal(SIGMINUS+blocks[i].signal, sighandler);
	}
#endif
}

int getstatus(char *str, char *last)
{
	strcpy(last, str);
	str[0] = '\0';
	for (unsigned int i = 0; i < LENGTH(blocks); i++)
		strcat(str, statusbar[i]);
	str[strlen(str)-strlen(delim)] = '\0';

	// Debug: log the status string with control characters
	FILE *f = fopen("/tmp/dwmblocks-status.log", "w");
	if (f) {
		fprintf(f, "Status string (len=%zu):\n", strlen(str));
		for (size_t i = 0; i < strlen(str); i++) {
			if (str[i] < 32)
				fprintf(f, "[%d]", (unsigned char)str[i]);
			else
				fprintf(f, "%c", str[i]);
		}
		fprintf(f, "\n");
		fclose(f);
	}

	return strcmp(str, last);//0 if they are the same
}

#ifndef NO_X
void setroot()
{
	if (!getstatus(statusstr[0], statusstr[1]))//Only set root if text has changed.
		return;
	XStoreName(dpy, root, statusstr[0]);
	XFlush(dpy);
}

int setupX()
{
	dpy = XOpenDisplay(NULL);
	if (!dpy) {
		fprintf(stderr, "dwmblocks: Failed to open display\n");
		return 0;
	}
	screen = DefaultScreen(dpy);
	root = RootWindow(dpy, screen);
	return 1;
}
#endif

void pstdout()
{
	if (!getstatus(statusstr[0], statusstr[1]))//Only write out if text has changed.
		return;
	printf("%s\n",statusstr[0]);
	fflush(stdout);
}


void statusloop()
{
	setupsignals();
	int i = 0;
	getcmds(-1);
	while (1) {
		getcmds(i++);
		writestatus();
		if (!statusContinue)
			break;
		sleep(1.0);
	}
}

#ifndef __OpenBSD__
/* this signal handler should do nothing */
void dummysighandler(int signum)
{
    return;
}
#endif

void sighandler_info(int signum, siginfo_t *si, void *ucontext)
{
	(void)signum;
	(void)ucontext;

	// Extract signal and button from value
	// dwm sends: sv.sival_int = 0 | (dwmblockssig << 8) | button
	int signal = 0;
	if (si) {
		button = si->si_value.sival_int & 0xFF;
		signal = (si->si_value.sival_int >> 8) & 0xFF;

		// Debug logging
		FILE *f = fopen("/tmp/dwmblocks-debug.log", "a");
		if (f) {
			fprintf(f, "Click received: signal=%d, button=%d, sival_int=%d\n",
			        signal, button, si->si_value.sival_int);
			fclose(f);
		}
	}

	// If it's a left click (button 1), execute the click command
	if (button == 1 && signal > 0) {
		for (unsigned int i = 0; i < LENGTH(blocks); i++) {
			if (blocks[i].signal == signal && blocks[i].clickcmd) {
				// Fork and execute the click command
				if (fork() == 0) {
					execl("/bin/sh", "sh", "-c", blocks[i].clickcmd, NULL);
					exit(0);
				}
				break;
			}
		}
	}

	if (signal > 0) {
		getsigcmds(signal);
		writestatus();
	}
}

void sighandler(int signum)
{
	// Handle real-time signals for block updates
	int signal = signum - SIGPLUS;
	getsigcmds(signal);
	writestatus();
}

void termhandler(int signum)
{
	statusContinue = 0;
}

int main(int argc, char** argv)
{
	for (int i = 0; i < argc; i++) {//Handle command line arguments
		if (!strcmp("-d",argv[i]))
			strncpy(delim, argv[++i], delimLen);
		else if (!strcmp("-p",argv[i]))
			writestatus = pstdout;
	}
#ifndef NO_X
	if (!setupX())
		return 1;
#endif
	delimLen = MIN(delimLen, strlen(delim));
	delim[delimLen++] = '\0';
	signal(SIGTERM, termhandler);
	signal(SIGINT, termhandler);
	statusloop();
#ifndef NO_X
	XCloseDisplay(dpy);
#endif
	return 0;
}
