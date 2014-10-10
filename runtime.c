/***************************************************************************
 *  Title: Runtime environment 
 * -------------------------------------------------------------------------
 *    Purpose: Runs commands
 *    Author: Stefan Birrer
 *    Version: $Revision: 1.1 $
 *    Last Modification: $Date: 2005/10/13 05:24:59 $
 *    File: $RCSfile: runtime.c,v $
 *    Copyright: (C) 2002 by Stefan Birrer
 ***************************************************************************/
/***************************************************************************
 *  ChangeLog:
 * -------------------------------------------------------------------------
 *    $Log: runtime.c,v $
 *    Revision 1.1  2005/10/13 05:24:59  sbirrer
 *    - added the skeleton files
 *
 *    Revision 1.6  2002/10/24 21:32:47  sempi
 *    final release
 *
 *    Revision 1.5  2002/10/23 21:54:27  sempi
 *    beta release
 *
 *    Revision 1.4  2002/10/21 04:49:35  sempi
 *    minor correction
 *
 *    Revision 1.3  2002/10/21 04:47:05  sempi
 *    Milestone 2 beta
 *
 *    Revision 1.2  2002/10/15 20:37:26  sempi
 *    Comments updated
 *
 *    Revision 1.1  2002/10/15 20:20:56  sempi
 *    Milestone 1
 *
 ***************************************************************************/
#define __RUNTIME_IMPL__
/************System include***********************************************/
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <termios.h>
/************Private include**********************************************/
#include "runtime.h"
#include "io.h"
#include "tsh.h"

/************Defines and Typedefs*****************************************/
/*  #defines and typedefs should have their names in all caps.
 *  Global variables begin with g. Global constants with k. Local
 *  variables should be in all lower case. When initializing
 *  structures and arrays, line everything up in neat columns.
 */

/************Global Variables*********************************************/

#define NBUILTINCOMMANDS (sizeof BuiltInCommands / sizeof(char*))



bgjobL *fgjob = NULL;

/* the pids of the background processes */
bgjobL *bgjobs = NULL;
/************Function Prototypes******************************************/
/* run command */
static void RunCmdFork(bgjobL*, bool);
/* runs an external program command after some checks */
static void RunExternalCmd(bgjobL*, bool);
/* resolves the path and checks for exutable flag */
static bool ResolveExternalCmd(commandT*);
/* forks and runs a external program */
static void Exec(bgjobL*, bool);
/* runs a builtin command */
static void RunBuiltInCmd(commandT*);
/* checks whether a command is a builtin command */
static bool IsBuiltIn(char*);
/************External Declaration*****************************************/

/**************Implementation***********************************************/
int total_task;
void RunCmd(commandT** cmd, int n)
{
  int i;
  total_task = n;
//  if(n == 1)
//    RunCmdFork(cmd[0], TRUE);
//  else{
//    RunCmdPipe(cmd[0], cmd[1]);
//    for(i = 0; i < n; i++)
//      ReleaseCmdT(&cmd[i]);
//  }
  bgjobL *job = NULL;
  procesS *pl = NULL;

  job = malloc(sizeof(bgjobL));
  pl = malloc(sizeof(procesS));

  job->first_process = pl;

//  dup2( STDIN_FILENO, job->stdin);
//  dup2(STDOUT_FILENO, job->stdout);
//  dup2(STDERR_FILENO, job->stderr);


  if(n == 1) {
    job->cmdline = strdup(cmd[0]->cmdline);
    pl->command = cmd[0];
//    RunCmdFork(job, TRUE);
  }
  else {
    i = 1;
    while(cmd[i] != NULL) {
      procesS *p = NULL;
      p = malloc(sizeof(procesS));
      p->command = cmd[i];
      pl->next = p;

      printf("pipeline mode enable\n");
      pl = p;
      i++;
    }
//    RunCmdFork(job, TRUE);
  }

  RunCmdFork(job, TRUE);


    
}

void RunCmdFork(bgjobL *job, bool fork)
{
  procesS *p;

  p = job->first_process;
  if (p->command->argc<=0)
    return;
  if (IsBuiltIn(p->command->argv[0]))
  {
    RunBuiltInCmd(p->command);
  }
  else
  {
    RunExternalCmd(job, fork);
  }
}

void RunCmdFg(bgjobL *job, int cont)
{
//  if(fgjob != NULL) RunCmdBg(fgjob, 0);
//  fgjob = job;
  
  tcsetpgrp(shell_terminal, job->pgid);
  
  if(cont) {
    tcsetattr(shell_terminal, TCSADRAIN, &job->tmodes);
    if(kill(- job->pgid, SIGCONT) < 0)
      perror("kill(SIGCONT)");
  }
  wait_for_job(job);

  tcsetpgrp(shell_terminal, shell_pgid);

  tcgetattr(shell_terminal, &job->tmodes);
  tcsetattr(shell_terminal, TCSADRAIN, &shell_tmodes);

}
void RunCmdBg(bgjobL *job, int cont)
{
  if(cont)
    if(kill(- job->pgid, SIGCONT) < 0)
      perror("kill (SIGCONT)");
  append(&bgjobs, job);
}

void RunCmdPipe(commandT* cmd1, commandT* cmd2)
{
}

void RunCmdRedirOut(commandT* cmd, char* file)
{
}

void RunCmdRedirIn(commandT* cmd, char* file)
{
}


/*Try to run an external command*/
static void RunExternalCmd(bgjobL *job, bool fork)
{
  procesS *p;
  p = job->first_process;
  
  if (ResolveExternalCmd(p->command)){
    Exec(job, fork);
  }
  else {
    printf("%s: command not found\n", p->command->argv[0]);
    fflush(stdout);
    ReleaseCmdT(&(p->command));
  }
}

/*Find the executable based on search list provided by environment variable PATH*/
static bool ResolveExternalCmd(commandT* cmd)
{
  char *pathlist, *c;
  char buf[1024];
  int i, j;
  struct stat fs;

  if(strchr(cmd->argv[0],'/') != NULL){
    if(stat(cmd->argv[0], &fs) >= 0){
      if(S_ISDIR(fs.st_mode) == 0)
        if(access(cmd->argv[0],X_OK) == 0){/*Whether it's an executable or the user has required permisson to run it*/
          cmd->name = strdup(cmd->argv[0]);
          return TRUE;
        }
    }
    return FALSE;
  }
  pathlist = getenv("PATH");
  if(pathlist == NULL) return FALSE;
  i = 0;
  while(i<strlen(pathlist)){
    c = strchr(&(pathlist[i]),':');
    if(c != NULL){
      for(j = 0; c != &(pathlist[i]); i++, j++)
        buf[j] = pathlist[i];
      i++;
    }
    else{
      for(j = 0; i < strlen(pathlist); i++, j++)
        buf[j] = pathlist[i];
    }
    buf[j] = '\0';
    strcat(buf, "/");
    strcat(buf,cmd->argv[0]);
    if(stat(buf, &fs) >= 0){
      if(S_ISDIR(fs.st_mode) == 0)
        if(access(buf,X_OK) == 0){/*Whether it's an executable or the user has required permisson to run it*/
          cmd->name = strdup(buf); 
          return TRUE;
        }
    }
  }
  return FALSE; /*The command is not found or the user don't have enough priority to run.*/
}

static void Exec(bgjobL *job, bool forceFork)
{
  procesS *p;
  p = job->first_process;
  int bg = p->command->bg;

  job->backg = bg;
  pid_t pid,pid1;               // pid1 is parent pid.
  int mypipe[2], infile, outfile;

  infile = job->stdin;
//  p = job->first_process;

  while(p != NULL) {
    if(p->next) {
      if(pipe(mypipe) < 0) {
        perror("pipe");
        exit(1);
      }
      outfile = mypipe[1];
    }
    else {
      outfile = job->stdout;
    }
    pid1 = getpid();
    pid = fork();
    int status;


    if(pid <0) {    /* error occurred */
      fprintf(stderr, "Fork Failed");
    }
    else if(pid == 0) {   // Child process
  //    execv(cmd->name, cmd->argv);
        launch_process(p, job->pgid, infile, outfile, job->stderr);    
    }
    else {
      p->pid = pid;
      if(shell_is_interactive) {
        if(!job->pgid)
          job->pgid = pid;
        setpgid(pid, job->pgid);
      }
    }

    if(infile != job->stdin)
      close(infile);
    if(outfile != job->stdout)
      close(outfile);
    infile = mypipe[0];
    p = p->next;
  }

  if(!shell_is_interactive)
    wait_for_job(job);
  if(bg)
    RunCmdBg(job, 0);
  else {
//    wait(NULL);
    RunCmdFg(job, 0);
  }
}

static bool IsBuiltIn(char* cmd)
{
  if(strcmp(cmd, "bg") == 0) 
    return TRUE;

  else if(strcmp(cmd, "jobs") == 0) return TRUE;
  else if(strcmp(cmd, "fg") == 0) return TRUE;
  else if(strcmp(cmd, "cd") == 0) return TRUE;
  else {
    return FALSE;     
  }
}


static void RunBuiltInCmd(commandT* cmd)
{
  bgjobL *b;
  if(strcmp(cmd->argv[0], "bg") == 0) {
    if(bgjobs != NULL){
      int i=1;
      for(b = bgjobs; b; b = b->next) {
          printf("[%d]+ %s &\n", i, b->cmdline );
          i++;
      }
    }
    else {
      printf("-tsh: bg: current: no such job\n");
   }
  }      
  else if(strcmp(cmd->argv[0], "jobs") == 0) {
     int i=0;
    bgjobL *current = NULL;
    if(bgjobs != NULL) {
      current = bgjobs;
      while(current != NULL) {
        i++;
        if(job_is_stopped(current)) {
          format_job_infor(current, "Stopped");
        }
        else if(job_is_completed(current)) {
          format_job_infor(current, "Done");
        }
        else {
          format_job_infor(current, "Running");
        }

        
        current = current->next;
      }
    }
  }
  else if(strcmp(cmd->argv[0], "fg") == 0) {
    bgjobL *j, *jlast, *jnext ;
    j = bgjobs;
    int i = 0;
    jlast = NULL;
    if(cmd->argv[1] != NULL) {
      while(j->jobid != atoi(cmd->argv[1])) {
        jlast = j;
        if(j->next != NULL) {
          j = j->next;
        }
        i++;
        if(i == 50) break;
      }
    }
//    printf("jlast: %x\n", jlast);
//    fflush(stdout);
    if(jlast)
      jlast->next = j->next;
    else
      bgjobs = j->next;
    continue_job(j, 1);
  }
  else if(strcmp(cmd->argv[0], "cd") == 0) {
    if(cmd->argv[1] != NULL) {
      
      chdir(cmd->argv[1]);
    }
    else {
      chdir(getenv("HOME"));
    }
  }
}



void CheckJobs()
{
  fflush(stdout);
  bgjobL *j, *jlast, *jnext;
  procesS *p;
  
  update_status();

  j = bgjobs;
  jlast = NULL;
  while(j != NULL) {
//    printf("j->cmdline: %s\n", j->cmdline);
    jnext = j->next;
//    printf("loop**** j => 0x%08x\n", j);

    if(job_is_completed(j)) {
//      printf("HEHE\n");
      fflush(stdout);
      format_job_infor(j, "Done");
      if(jlast)
        jlast->next = jnext;
      else
        bgjobs = jnext;
      free_job(&j);
    }

//    else if(job_is_stopped (j)) {
//      format_job_infor(j, "Stopped");
//      jlast = j;
//    }
    
    
    else {
      jlast = j;
    }
    j = jnext;
  }
}




commandT* CreateCmdT(int n)
{
  int i;
  commandT * cd = malloc(sizeof(commandT) + sizeof(char *) * (n + 1));
  cd -> name = NULL;
  cd -> cmdline = NULL;
  cd -> is_redirect_in = cd -> is_redirect_out = 0;
  cd -> redirect_in = cd -> redirect_out = NULL;
  cd -> argc = n;
  for(i = 0; i <=n; i++)
    cd -> argv[i] = NULL;
  return cd;
}

/*Release and collect the space of a commandT struct*/
void ReleaseCmdT(commandT **cmd){
  int i;
  if((*cmd)->name != NULL) free((*cmd)->name);
  if((*cmd)->cmdline != NULL) free((*cmd)->cmdline);
  if((*cmd)->redirect_in != NULL) free((*cmd)->redirect_in);
  if((*cmd)->redirect_out != NULL) free((*cmd)->redirect_out);
  for(i = 0; i < (*cmd)->argc; i++)
    if((*cmd)->argv[i] != NULL) free((*cmd)->argv[i]);
  free(*cmd);
}

void launch_process(procesS *p, pid_t pgid, int infile, int outfile, int errfile) {
  pid_t pid;

  if(shell_is_interactive) {
    pid = getpid();
    if(pgid == 0) pgid = pid;
    setpgid(pid, pgid);
    if(!p->command->bg) {
      tcsetpgrp(shell_terminal, pgid);
    }
    else {}
      
    
    signal(SIGINT, SIG_DFL);
    signal(SIGQUIT, SIG_DFL);
    signal(SIGTSTP, SIG_DFL);
    signal(SIGTTIN, SIG_DFL);
    signal(SIGTTOU, SIG_DFL);
    signal(SIGCHLD, SIG_DFL);
  }
//  printf("cmd->name: %s\n", cmd->name);
/*
  if(infile != STDIN_FILENO) {
    dup2(infile, STDIN_FILENO);
    close(infile);
  }
  if(outfile != STDOUT_FILENO) {
    dup2(outfile, STDOUT_FILENO);
    close(outfile);
  }
  if(errfile != STDERR_FILENO) {
    dup2(errfile, STDERR_FILENO);
    close(errfile);
  }
*/
//  printf("cmd->name: %s, cmd->argv: %s\n", cmd->name, *(cmd->argv));
  execv(p->command->name, p->command->argv);
  perror("execv");
  exit(1);
}

void wait_for_job(bgjobL *job)
{
  int status;
  pid_t pid;

  do
    pid = waitpid (WAIT_ANY, &status, WUNTRACED);
  while ((mark_process_status(pid, status) == 0)
          && !job_is_stopped(job)
          && !job_is_completed(job));
}

int mark_process_status (pid_t pid, int status) 
{
  bgjobL *j;
  procesS *p;
  if(pid>0) {
    j = bgjobs;
    while(j != NULL) {
      p = j->first_process;
      while(p != NULL) {
        if(p->pid == pid) {
          p->status = status;
          if(WIFSTOPPED(status))
            p->stopped = 1;
          else {
            p->completed = 1;
            if(WIFSIGNALED(status))
              fprintf(stderr, "%d: Terminated by signal %d.\n", (int)pid, WTERMSIG(p->status));
          }
          return 0;
        }
        p = p->next;
      }
      j = j->next;
    }
//    fprintf(stderr, "No child process %d.\n", pid);
    return 1;
  }
  else if(pid == 0||errno == ECHILD)
    return 1;
  else {
    perror("waitpid");
    return 1;
  }
}

void update_status(void) {
  int status;
  pid_t pid;

  do
    pid = waitpid(WAIT_ANY, &status, WUNTRACED|WNOHANG);
  while(mark_process_status(pid, status) == 0);
}

int length(bgjobL *head) {
  int count=0;
  bgjobL *current = head;

  while(current != NULL) {
    count++;
    current = current->next;
  }
  return(count);
}

void append(bgjobL **headRef, bgjobL *job) {
  bgjobL *current = *headRef;
  int i = 1;



  if(current == NULL) {
    *headRef = job;
    job->jobid = 1;
  }
  else {
    i=2;
    while(current->next != NULL) {
      if(job->jobid != NULL) {
        i = current->jobid + 1;
      }
      current = current->next;
      
    }
    current->next = job;
    job->jobid = i;
  }
}

bgjobL *find_job(pid_t pgid) {
  bgjobL *j;
  
  if(bgjobs != NULL) {
    j = bgjobs;
    while(j != NULL) {
      if(j->pgid == pgid)
        return j;
      j = j->next;
    }
  }
  return NULL;
}

int job_is_stopped (bgjobL *j) {
  procesS *p;

  p = j->first_process;
  while(p != NULL) {
    if(!p->completed && !p->stopped) return 0;
    p = p->next;
  }
  return 1;
}

int job_is_completed(bgjobL *j) {
  procesS *p;

  p = j->first_process;
  while(p != NULL) {
    if(!p->completed) 
      return 0;
    p = p->next;
  }
  return 1;
}
      
void format_job_infor(bgjobL *j, const char *status) {
  int i = 1;
  bgjobL *current = bgjobs;

  while(current != j) {
    current = current->next;
    i++;
  }
 
  if(strcmp(status, "Done")==0) {
    fprintf(stdout, "[%d]\t%s\t\t\t%s\n", j->jobid, status, j->cmdline);
  }
  else{
    fprintf(stdout, "[%d]\t%s\t\t\t%s &\n", j->jobid, status, j->cmdline);
  }
  fflush(stdout);
}

void free_job(bgjobL **j) {
//  printf("free_job => 0x%08x\n", *j);
  if((*j)==bgjobs) bgjobs = NULL;
//  if((*j)->cmdline != NULL) free((*j)->cmdline);  //TODO probably has seg fault. to delete
//  if((*j)->first_process != NULL) {
//    if((*j)->first_process->command != NULL) {
//      ReleaseCmdT(&((*j)->first_process->command));
//    }
//    free((*j)->first_process);
//  }
 // if((*j)->next != NULL) free((*j)->next);
  free(*j);
}

void mark_job_as_running(bgjobL *j) {
  procesS *p;

  p=j->first_process;
  while(p != NULL){
    p->stopped = 0;
    p = p->next;
  }
}

void continue_job( bgjobL *j, int foreground) {
  mark_job_as_running(j);
  if(foreground==1){
    RunCmdFg(j, 1);
  }
  else {
    RunCmdBg(j, 1);
  }
}

