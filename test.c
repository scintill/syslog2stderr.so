#include <syslog.h>

int main(int argc, char *argv[]) {
	openlog("test", 0, LOG_USER);
	syslog(LOG_NOTICE, "testing 1 2 3");

	return 0;
}
