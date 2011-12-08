#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>

int ll_dir(const char *fn)
{
	int ret = ENOENT;
	DIR *dp;

	if (!fn)
		return ret;

	if (!(dp = opendir(fn))) {
		ret = errno;
		return ret;
	}
	sleep(4);
	closedir(dp);
	return 0;
}

int main(int argc, char **argv)
{
	int ret;

	ret = ll_dir(argv[1]);
	if (ret) {
		printf("%s %s(%d)\n", argv[1], strerror(ret), ret);
	}
	
	return ret;
}
