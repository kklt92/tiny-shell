/***************************************************************************
 *  Title: MySimpleShell 
 * -------------------------------------------------------------------------
 *    Purpose: A simple shell implementation 
 *    Author: Stefan Birrer
 *    Version: $Revision: 1.1 $
 *    Last Modification: $Date: 2005/10/13 05:24:59 $
 *    File: $RCSfile: tsh.c,v $
 *    Copyright: (C) 2002 by Stefan Birrer
 ***************************************************************************/
/***************************************************************************
 *  ChangeLog:
 * -------------------------------------------------------------------------
 *    $Log: tsh.c,v $
 *    Revision 1.1  2005/10/13 05:24:59  sbirrer
 *    - added the skeleton files
 *
 *    Revision 1.4  2002/10/24 21:32:47  sempi
 *    final release
 *
 *    Revision 1.3  2002/10/23 21:54:27  sempi
 *    beta release
 *
 *    Revision 1.2  2002/10/15 20:37:26  sempi
 *    Comments updated
 *
 *    Revision 1.1  2002/10/15 20:20:56  sempi
 *    Milestone 1
 *
 ***************************************************************************/
#define __MYSS_IMPL__

/************System include***********************************************/
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <termios.h>

/************Private include**********************************************/
#include "tsh.h"
#include "io.h"
#include "interpreter.h"
#include "runtime.h"

/************Defines and Typedefs*****************************************/
/*  #defines and typedefs should have their names in all caps.
 *  Global variables begin with g. Global constants with k. Local
 *  variables should be in all lower case. When initializing
 *  structures and arrays, line everything up in neat columns.
 */

#define BUFSIZE 80

/************Global Variables*********************************************/
pid_t shell_pgid;
struct termios shell_tmodes;
int shell_terminal;
int shell_is_interactive = 1;
static sigset_t block_set, old_block_set;

/************Function Prototypes******************************************/
/* handles SIGINT and SIGSTOP signals */	
static void sig(int);

/************External Declaration*****************************************/

/**************Implementation***********************************************/

int main (int argc, char *argv[])
{
  /* Initialize command buffer */
  char* cmdLine = malloc(sizeof(char*)*BUFSIZE);
/*  char* input[] = {"bash -c \"sleep 2; echo hello2;\" &",
                    "echo hello",
                    "bash -c \"sleep 1; echo hello3;\" &",
                    "echo hello4",
                    "sleep 3",
                    "jobs"};*/

/*  char* input[] = {"./testsuite/myspin 5 &",
                    "./testsuite/myspin 3 &",
                    "./testsuite/myspin 1 &"};*/

/*  char *input[] = {"bash -c \"sleep 2; echo hello1;\" &",
                    "bash -c \"sleep 4; echo hello2;\" &",
                    "jobs",
                    "sleep 3",
                    "jobs",
                    "sleep 2",
                    "jobs",
                    "bash -c \"sleep 4; echo hello3;\" &",
                    "jobs",
                    "sleep 5",
                    "jobs"};*/

  char *input[] = {"echo &", " "};

  int i = 0;

  shell_terminal = STDIN_FILENO;
//shell_is_interactive = isatty(shell_terminal);

//  while(tcgetpgrp(shell_terminal) != (shell_pgid = getpgrp())) {
//    kill(-shell_pgid, SIGTTIN);
//  }


//  printf("shell_terminal: %d\n", shell_terminal);
//  printf("shell_is_interactive: %d\n", shell_is_interactive);
  /* shell initialization */
  if (signal(SIGINT, sig) == SIG_ERR) PrintPError("SIGINT");
  if (signal(SIGTSTP, sig) == SIG_ERR) PrintPError("SIGTSTP");
  if (signal(SIGCHLD, sig) == SIG_ERR) PrintPError("SIGCHLD");

  signal(SIGTTOU, SIG_IGN);
  signal(SIGTTIN, SIG_IGN);

  initial_signal();
 
  shell_pgid = getpid();
  if(setpgid(shell_pgid, shell_pgid) < 0)
  {
    PrintPError("Couldn't put the shell in its own process group");
    exit(1);
  }

  tcsetpgrp(shell_terminal, shell_pgid);
  tcgetattr(shell_terminal, &shell_tmodes);
  
//  printf("shell_termnial now is: %d, the shell_pgid is: %d\n", shell_terminal, shell_pgid); 

  while (!forceExit) /* repeat forever */
  {

    signal_mask();
//    printf("%s> ", SHELLNAME);
    fflush(stdout);
    /* read command line */
    if(i < 0)
      strcpy(cmdLine, input[i]);
    else
      getCommandLine(&cmdLine, BUFSIZE);

//    printf("========================================\n");
//    printf("input: %s\n", cmdLine);
    if(strcmp(cmdLine, "exit") == 0)
    {
      forceExit=TRUE;
      continue;
    }

    reset_signal_mask();
    /* checks the status of background jobs */
    CheckJobs();

    /* interpret command and line
     * includes executing of commands */
    Interpret(cmdLine);

//    printf("%s\n", cmdLine);
    
    *cmdLine = NULL;
    i++;
  }

  /* shell termination */
  free(cmdLine);
  return 0;
} /* end main */

static void sig(int signo)
{
  if(signo == SIGINT) {
    if(bgjobs != NULL) {
      bgjobL *j;
      j = bgjobs;
      while((j != NULL) && (j->backg == 1)) {
        j = j->next;
      }
      if(j->pgid > 0 && j!= NULL && j->backg == 0)
        kill(-j->pgid, SIGINT);
    }
  }


  if(signo == SIGTSTP) {
//    printf("recieved IGTSTP\n");
//    fflush(stdout);
        if(fgjob != NULL) {
        kill(fgjob->pgid, SIGTSTP);
        format_job_infor(fgjob, "Stopped");
        RunCmdBg(fgjob, 0);
      }
    }
      
  
  if(signo == SIGCHLD) {
    int status;
    pid_t pid;
    while((pid = waitpid(-1, &status, WNOHANG)) > 0) {
    }
    }
}


void initial_signal() {
  sigemptyset(&block_set);
  sigemptyset(&old_block_set);
  sigaddset(&block_set, SIGCHLD);
}

void signal_mask() {
  sigprocmask(SIG_SETMASK, &block_set, &old_block_set);
}

void reset_signal_mask() {
  sigprocmask(SIG_SETMASK, &block_set, NULL);
}

