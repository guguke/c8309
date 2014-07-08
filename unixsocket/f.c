#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <sys/wait.h>
#include <stdlib.h>

int var_glb; /* A global variable*/


void sigchld_handler(int s)

{

	printf("  start wait \n");
	while(wait(NULL) > 0);
	printf("  end wait \n");

}

 
int main(void)
{
	pid_t childPID;
	int var_lcl = 0;
	int i;
	struct sigaction sa;
	/* clean all the dead processes */

	sa.sa_handler = sigchld_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;

	if(sigaction(SIGCHLD, &sa, NULL) == -1)
	{
		perror("Server-sigaction() error");
		exit(1);
	}
	else
		printf("Server-sigaction() is OK...\n");	

	childPID = fork();

	if(childPID >= 0) // fork was successful
    {
        if(childPID == 0) // child process
        {
            var_lcl++;
            var_glb++;
            printf("\n Child Process :: var_lcl = [%d], var_glb[%d]\n", var_lcl, var_glb);
			for(i=0;i<2;i++){
				sleep(5);
				printf(" pid0 : sleep5 : %d \n",i+10);
			}
        }
        else //Parent process
        {
            var_lcl = 10;
            var_glb = 20;
            printf("\n Parent process :: var_lcl = [%d], var_glb[%d]\n", var_lcl, var_glb);
			for(i=0;i<9;i++){
				sleep(4);
				printf(" pid p : sleep4 : %d \n",i+20);
			}
        }
    }
    else // fork failed
    {
        printf("\n Fork failed, quitting!!!!!!\n");
        return 1;
    }

	printf(" exit \n");
    return 0;
}


