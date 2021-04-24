#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<unistd.h>
#include<sys/sysmacros.h>
#include<sys/wait.h>

int main()
{
	//create device files
	if(mknod("./null", S_IFCHR | S_IRUSR | S_IWUSR, makedev(1, 3)) == -1)
	{
		perror("mknod() failed");
	}
	if(mknod("/dev/ttyS0", S_IFCHR | S_IRUSR | S_IWUSR, makedev(4, 64)) == -1)
	{
		perror("mknod() failed");
	}
	if(mknod("/dev/ttyAMA0", S_IFCHR | S_IRUSR | S_IWUSR, makedev(204, 64)) == -1)
	{
		perror("mknod() failed");
	}
	if(mknod("/dev/fb0", S_IFCHR | S_IRUSR | S_IWUSR, makedev(29, 0)) == -1)
	{
		perror("mknod() failed");
	}
	printf("here\n\n");
	//call 3 test procedures
	if(fork() == 0)
	{
		if((execl("/tools/binary/1","1","execl",NULL)) == -1)
		{
			perror("execl");
			exit(1);
		}
	}
	sleep(3);
	if(fork() == 0)
	{
		if((execl("/tools/binary/2","2","execl",NULL)) == -1)
		{
			perror("execl");
			exit(1);
		}
	}
	sleep(3);
	if(fork() == 0)
	{
		if((execl("/tools/binary/3","3","execl",NULL)) == -1)
		{
			perror("execl");
			exit(1);
		}
	}
	while(1);
	return 0;
}
