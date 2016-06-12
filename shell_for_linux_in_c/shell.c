#include "LineParser.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#define PATH_MAX  4096
#define USER_INPUT_MAX  2048
#define history 100

char *History[history],*homeDir;
int number_of_command = -1,**pipes;

typedef struct pair
{
  struct pair *next;
  int name_size;
  int val_size;
  char *name;
  char *value;
}pair;

struct pair *enviorment = NULL ,*last_in_env = NULL;

struct pair * get_value_by_name(char *name){
    int ser_size = strlen(name);
    struct pair *temp = enviorment;
    while(temp){
      if(ser_size+1 == temp->name_size && !strncmp(name,temp->name,ser_size)){
	return temp;
      }
      temp = temp->next;
    }
    return NULL;
}

void add_to_env(char *name,char *value){
    int n_size = strlen(name)+1,v_size =strlen(value)+1;
    struct pair *new_var;
    if((new_var = get_value_by_name(name))){
      free(new_var->value);
      new_var->value = (char *)malloc(v_size);
      new_var->value[v_size-1] = '\0';
      strncpy(new_var->value,value,v_size);
      return;
    }
    
    new_var = (struct pair*)malloc(sizeof(struct pair*)+2*sizeof(int)+2*sizeof(char*));
    new_var->next = NULL;
    new_var->name_size = n_size;
    new_var->val_size = v_size;
    new_var->name = (char *)malloc(n_size);
    strncpy(new_var->name,name,n_size);
    new_var->name[n_size-1]='\0';
    new_var->value = (char *)malloc(v_size);
    strncpy(new_var->value,value,v_size);
    new_var->value[v_size-1] = '\0';
    
    if(last_in_env)
      last_in_env->next = new_var;
    else
      enviorment = new_var;
    last_in_env = new_var;
}


void deleteVar(char *name){
  struct pair *temp = enviorment,*previous = NULL;
  int ser_size = strlen(name);
    while(temp){
      if(ser_size+1 == temp->name_size && !strncmp(name,temp->name,ser_size)){
	if(previous){
	    previous->next = temp->next;
	    if(!previous->next){
		   last_in_env = previous;
	    }
	}else{
	  enviorment = temp->next;
	  if(!enviorment){
		   last_in_env = enviorment;
	  }
	}
	free(temp->name);
	free(temp->value);
	free(temp);
	return;
      }
      previous = temp;
      temp = temp->next;
    }
}

void printEnv(){
    struct pair *temp = enviorment;
    while(temp){
      printf("%s = %s\n",temp->name,temp->value);
      temp = temp->next;
    }
}

void freeHistory(int current_place){
  for(; number_of_command > -1 ; number_of_command--)
    free(History[number_of_command]);
}

void freeEnv(){
  struct pair *temp1 = enviorment,*temp2;
  while(temp1){
    free(temp1->name);
    free(temp1->value);
    temp2 = temp1->next;
    free(temp1);
    temp1 = temp2;
  }
}

int **createPipes(int nPipes){
  if(nPipes>0){
    int **pipes,i;
    pipes = (int **)malloc(4*nPipes);
    for(i=0 ;i < nPipes ; i++){
      pipes[i] = (int *)malloc(8);
      if(pipe(pipes[i]) == -1){
	perror("pipe");
	exit(0);
      }
    }
    return pipes;
  }
  return NULL;
} 

void releasePipes(int **pipes, int nPipes){
  int i;
  for(i=0;i < nPipes;i++)
    free(pipes[i]);
  free(pipes);
}

int *leftPipe(int **pipes, cmdLine *pCmdLine) {
  if(pCmdLine->idx > 0){
    return pipes[pCmdLine->idx-1];
  }
  return NULL;
}

int *rightPipe(int **pipes, cmdLine *pCmdLine) {
  if(pCmdLine->next){
    return pipes[pCmdLine->idx];
  }
  return NULL;
}

void execute(cmdLine *line){
  int pid,his = 0,index,status;
  if(!line)
    return;
  for(index = 0 ; index < line->argCount ; index++){
    if(line->arguments[index][0]=='$'){
      struct pair *temp = get_value_by_name(line->arguments[index]+1);
      replaceCmdArg(line, index, temp->value);
    }
  }
  if(!strncmp(line->arguments[0],"cd",2)){
      char *toAdd;
      if(line->arguments[1][0] == '~') toAdd = homeDir;
      else toAdd = line->arguments[1];
      if(chdir(toAdd)==-1)
	perror("Error in execute cd");
      execute(line->next);
      return;
  }else if(!strncmp(line->arguments[0],"history",7)){
    if(number_of_command==-1)
      printf("No history");
    else
      while(his <= number_of_command){
	printf("#%d\t%s\n",his,History[his]);
	his++;
      } 
    execute(line->next);
      return;
  }else if(!strncmp(line->arguments[0],"set",3)){
    add_to_env(line->arguments[1],line->arguments[2]);
    execute(line->next);
    return;
  }else if(!strncmp(line->arguments[0],"env",3)){
    printEnv();
    execute(line->next);
    return;
  }else if(!strncmp(line->arguments[0],"delete",6)){
    deleteVar(line->arguments[1]);
    execute(line->next);
    return;
  }
  if(!(pid = fork())){
    if(line->inputRedirect){
      fclose(stdin);
      fopen(line->inputRedirect,"r"); 
    }
    if(line->outputRedirect){
      fclose(stdout);
      fopen(line->outputRedirect,"w");
    }
    execvp(line->arguments[0],line->arguments);
    perror("execute faild");
    freeHistory(number_of_command);
    freeEnv();
    _exit(1);
  }else{
    if(line->blocking)
      waitpid(pid,&status,0);
  }
}

int getSubIndex(char * prefix)
{
  int i,pre_len = strlen(prefix)-1;
  for(i=0;i<number_of_command;i++)
    if(!strncmp(prefix,History[i],pre_len))
      return i;
  return -1;
}

int countLines(cmdLine *cmdline){
  cmdLine *tmp = cmdline;
  while(tmp->next)
    tmp = tmp->next;
  return tmp->idx;
}

void callMyFather(cmdLine *cmd_line,int kind){
      int pid = fork();
      if(pid == -1){
        perror("fork");
        _exit(0);
      }

      if(pid){    /* Parent reads from pipe */
	if(kind == 1){
	  close(rightPipe(pipes,cmd_line)[1]);
	  waitpid(pid,NULL,0);
	  if(cmd_line->next->next)
	    callMyFather(cmd_line->next,2);
	  else callMyFather(cmd_line->next,3);
	}
	else if(kind==2){
	  close(rightPipe(pipes,cmd_line)[1]);
	  close(leftPipe(pipes,cmd_line)[0]);
	  waitpid(pid,NULL,0);
	  if(cmd_line->next->next)
	    callMyFather(cmd_line->next,2);
	  else callMyFather(cmd_line->next,3);
	}else{
	  close(leftPipe(pipes,cmd_line)[0]);
	  waitpid(pid,NULL,0);
	}
	  
      }else{            /* Child1 writes argv[1] to pipe */
	if(kind == 1){
	  fclose(stdout);
	  dup(rightPipe(pipes,cmd_line)[1]);
	  close(rightPipe(pipes,cmd_line)[1]);
	}
	else if(kind==2){
	  fclose(stdin);
	  dup(leftPipe(pipes,cmd_line)[0]);
	  close(leftPipe(pipes,cmd_line)[0]);
	  fclose(stdout);
	  dup(rightPipe(pipes,cmd_line)[1]);
	  close(rightPipe(pipes,cmd_line)[1]);
	}else{
	  fclose(stdin);
	  dup(leftPipe(pipes,cmd_line)[0]);
	  close(leftPipe(pipes,cmd_line)[0]);
	  
	}
        execute(cmd_line);
        _exit(1);
      }
}

int main(){
  char cur_address[PATH_MAX],user_input[USER_INPUT_MAX];
  cmdLine *cur_cmdLine;
  int work=1,sub_index,to_sub = 0,run = 1;
  homeDir = getenv("HOME");
  while(work){
    getcwd(cur_address,PATH_MAX);
    printf("%s> ",cur_address);
    
    fgets(user_input,USER_INPUT_MAX,stdin);
    if(feof(stdin)){
      freeHistory(number_of_command);
      perror("Error at reading stdin");
      exit(0);
    }
    if(!strncmp(user_input,"quit",4)){
      work = 0;
      run = 0;
    }
    number_of_command++;
    if(number_of_command < history)
    {
      if(user_input[0]!='!'){
       int u_i_l = strlen(user_input)+1;
       History[number_of_command] = malloc(u_i_l);
       strncpy(History[number_of_command],user_input,u_i_l);
       History[number_of_command][u_i_l-1]='\0';
      }else{
       sub_index = getSubIndex(user_input+1);
       if(sub_index>-1){
	int h_l =  strlen(History[sub_index])+1;
	History[number_of_command] = malloc(h_l);
	strncpy(History[number_of_command],History[sub_index],h_l);
	History[number_of_command][h_l-1]='\0';
	to_sub = 1;
       }else
	 perror("Not in history");
      }
    }else
      perror("out of bounds");
    if (to_sub) cur_cmdLine = parseCmdLines(History[sub_index]);
    else cur_cmdLine = parseCmdLines(user_input);
    to_sub = 0;
    int lineCount = countLines(cur_cmdLine);
    if(run){
      if((pipes = createPipes(lineCount))){
	callMyFather(cur_cmdLine,1);
	releasePipes(pipes,lineCount);
      }else{
	execute(cur_cmdLine);
      }
    }
    freeCmdLines(cur_cmdLine);
  }
  freeHistory(number_of_command);
  freeEnv();
  return 1;
}