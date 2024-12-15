#include <signal.h>
#include <stdio.h>

#include "extern.h"

volatile sig_atomic_t sig_recv = 0;

void
handle_sig(int sig)
{
	(void)sig;
	sig_recv = 1;
}

int
install_handlers()
{
	struct sigaction sa;

	sa.sa_handler = handle_sig;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	if (sigaction(SIGINT, &sa, NULL) == -1) {
		perror("sigaction SIGINT handler");
		return -1;
	}
	if (sigaction(SIGQUIT, &sa, NULL) == -1) {
		perror("sigaction SIGQUIT handler");
		return -1;
	}
	if (sigaction(SIGTSTP, &sa, NULL) == -1) {
		perror("sigaction SIGTSTP");
		return -1;
	}
	return 0;
}