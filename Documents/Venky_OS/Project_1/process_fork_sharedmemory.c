#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/stat.h>

#define MAX_NUM_PROCESS 5			/* Max number of child processes */

main()
{
	int shared_mem_id, shared_mem_id_parent, i;
	int *shmaddr = NULL;
	pid_t ppid = getpid();			/* Save the parent PID for future */
	pid_t ret_pid, child_pid=0;

	key_t key = ftok("/", 'R'); 		/* Using this key, child processes identifies the shared memory used by the parent */

	/* This is parent process */

	/* Create and open a shared memory that will be shared between 
	 * the parent and all the child processes.
	 * Child processes increment the variable in shared memory.
	 * Also they decrement second variable when they terminate
	 * so that parent can determine when the last process terminated
	 */
	shared_mem_id_parent = shmget(key, 2*sizeof(int), IPC_CREAT  |  S_IRUSR  |  S_IWUSR);
	/* Attach the created physical memory to the process address space */
	shmaddr = (int *)shmat(shared_mem_id_parent, NULL, 0);
	if(shmaddr < 0)
	{
		perror("ERROR : Shmat failed\n");
		goto ERROR;
	}

	shmaddr[0] = 0;			/* Each process keeps incrementing the value */
	shmaddr[1] = MAX_NUM_PROCESS;	/* Each process keeps decrementing the value as they terminate*/

	for(i = 1; i <= MAX_NUM_PROCESS; i++)
	{
		if((ret_pid = fork()) == 0)
		{
			/* Child code */
			/* Open the created shared memory */
			shared_mem_id = shmget(key, 2*sizeof(int), S_IRUSR  |  S_IWUSR);
			/* Attach the shared memory to the child process address space */
			shmaddr = (int *)shmat(shared_mem_id, NULL, 0);

			if(shmaddr < 0)
			{
				perror("ERROR : Shmat failed\n");
				goto ERROR;
			}
			printf("This is child (j) = %d and pid = %d\n", ++*shmaddr, getpid());
			break;
		}
		else
			continue;
	}

	/* Parent process waits until all the child processes have terminated */
	while(ppid == getpid() && *(shmaddr+1) != 0)
		usleep(10);

	/* Decrements when ever a child process terminates */
	(*(shmaddr+1))--;

	if(ppid == getpid())
	{
		shmctl(shared_mem_id_parent, IPC_RMID, 0); /* Parent process releases created shared memory back to the OS */
		printf("\nAll the 5 child processes has terminated\nNow terminating their parent process\n\n");
	}
ERROR:
	exit(0);
}
