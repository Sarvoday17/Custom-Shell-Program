/********************************************************************************************
This is a template for assignment on writing a custom Shell. 

Students may change the return types and arguments of the functions given in this template,
but do not change the names of these functions.

Though use of any extra functions is not recommended, students may use new functions if they need to, 
but that should not make code unnecessorily complex to read.

Students should keep names of declared variable (and any new functions) self explanatory,
and add proper comments for every logical step.

Students need to be careful while forking a new process (no unnecessory process creations) 
or while inserting the signal handler code (which should be added at the correct places).

Finally, keep your filename as myshell.c, do not change this name (not even myshell.cpp, 
as you do not need to use any features for this assignment that are supported by C++ but not by C).
*********************************************************************************************/
// BT20CSE050 = Sarvoday Jadhav

#include <stdio.h>
#include <string.h>
#include <stdlib.h>			// exit()
#include <unistd.h>			// fork(), getpid(), exec()
#include <sys/wait.h>		// wait()
#include <signal.h>			// signal()
#include <fcntl.h>			// close(), open()




/*parseInput fn will check what type of cmd is to be executed and split commands into an array like
	arr[0]=cmd1
	arr[1]=cmd2...arr[n-1]=NULL
	This parsed array will be returned.
	THis checks for presence of ## or && or >.
	Any occurences of the above symbols within ""(inverted commas) will be ignored as they can be arguments
	to some shell command.Hence ,the code is lengthy to support this.
*/
char** parseInput(char *commands,int *type)
{
	// This function will parse the input string into multiple commands or a single command with arguments depending on the delimiter (&&, ##, >, or spaces).
	if(commands!=NULL)
	{
		int mode=0;
		/*
			0->single cmd
			1->parallel
			2->sequential
			3->redirection single
		*/
		int parallel=0,sequential=0,prev_hash=0,prev_and=0,isRedir=0,isCommaOpen=0,emptyCheck=0;
		int i=0,len=strlen(commands),error=0;
		for(i=0;i<len&&error==0;i++)//currently, #& and &# are allowed,but add conditon to parse them as wrong
		{
			if(commands[i]!=' ')
			{
				emptyCheck=1;
			}
			if(isCommaOpen==0)
			{
				if(commands[i]=='#')
				{
					if(prev_hash==0)
					{
						prev_hash=1;
					}
					else if(prev_hash==1)
					{
						sequential++;
						prev_hash++;
					}
					else
					{
						error=1;
					}
					prev_and=0;
				}
				else if(commands[i]=='&')
				{
					if(prev_and==0)
					{
						prev_and=1;
					}
					else if(prev_and==1)
					{
						parallel++;
						prev_and++;
					}
					else
					{
						error=1;
					}
					prev_hash=0;
				}
				else if(commands[i]=='>')
				{
					isRedir=1;
					prev_and=0;
					prev_hash=0;
				}
				else if(commands[i]=='"')
				{
					isCommaOpen=1;
					prev_hash=0;
					prev_and=0;
				}
				else
				{
					prev_hash=0;
					prev_and=0;
				}
			}
			else
			{
				if(commands[i]=='"')
				{
					isCommaOpen=0;
					prev_hash=0;
					prev_and=0;
				}
			}
		}
		if(parallel==0&&sequential==0)
		{
			mode=(isRedir==1)?3:0;
		}
		else if(parallel==0&&isRedir==0)
		{
			mode=2;
		}
		else if(sequential==0&&isRedir==0)
		{
			mode=1;
		}
		else
		{
			error=1;
		}
		//DEBUG STUFF
		if(error==0)
		{
			mode=(emptyCheck==1)?mode:-1;//-1 signifies empty string
			
		}
		else
		{
			
			printf("Shell: Incorrect command\n");
		}
		*type=mode;
		//Want to separate into commands here
		if(error==0)
		{
			char **separate_cmds=NULL;
			if(mode==0||mode==3)
			{
				separate_cmds=(char **)malloc(sizeof(char *)*(2));
				separate_cmds[0]=strdup(commands);
				separate_cmds[1]=NULL;
			}
			else if(mode==2)
			{
				separate_cmds=(char **)malloc(sizeof(char *)*(sequential+1+1));//seq+1 is no.of cmds,+1 for NULL
				int tokens=0;
				int isCommaOpen=0;
				int prev_hash=0;
				int i=0,found=0,start_loc=0;//to iterate
				while(tokens<sequential+1)
				{
					//Find the ## not inside a string
					found=0;
					for(;i<len&&found==0;i++)
					{
						if(isCommaOpen==0)
						{
							if(commands[i]=='"')
							{
								isCommaOpen=1;
							}
							else if(commands[i]=='#')
							{
								if(prev_hash==1)
								{
									//This is where the first arg ends
									commands[i-1]='\0';
									found=1;
								}
								else
								{
									prev_hash=1;
								}
							}
							else
							{
								prev_hash=0;
							}
						}
						else
						{
							if(commands[i]=='"')
							{
								isCommaOpen=0;
							}	
						}
					}
					separate_cmds[tokens++]=strdup(commands+start_loc);
					//printf("\nString identified:%s",separate_cmds[tokens-1]);
					//commands[i-2]='#';
					start_loc=i;
				}
				separate_cmds[tokens]=NULL;
			}
			else if(mode==1)
			{
				separate_cmds=(char **)malloc(sizeof(char *)*(parallel+1+1));//para+1 is no.of cmds,+1 for NULL
				int tokens=0;
				int isCommaOpen=0;
				int prev_and=0;
				int i=0,found=0,start_loc=0;//to iterate
				while(tokens<parallel+1)
				{
					//Find the ## not inside a string
					found=0;
					for(;i<len&&found==0;i++)
					{
						if(isCommaOpen==0)
						{
							if(commands[i]=='"')
							{
								isCommaOpen=1;
							}
							else if(commands[i]=='&')
							{
								if(prev_and==1)
								{
									//This is where the first arg ends
									commands[i-1]='\0';
									found=1;
								}
								else
								{
									prev_and=1;
								}
							}
							else
							{
								prev_and=0;
							}
						}
						else
						{
							if(commands[i]=='"')
							{
								isCommaOpen=0;
							}	
						}
					}
					separate_cmds[tokens++]=strdup(commands+start_loc);
					
					//commands[i-2]='#';
					start_loc=i;
				}
				separate_cmds[tokens]=NULL;
			}
			return separate_cmds;
		}
		return NULL;
	}
	return NULL;
}

int wordcount(char *str)
{
	int words=0,space=1,isCommaOpen=0;
	int i=0,len=strlen(str);
	for(;i<len;i++)
	{
		if(isCommaOpen==0)
		{
			if(str[i]=='"')
			{
				isCommaOpen=1;
			}
			if(space==1)
			{
				if(str[i]!=' ')
				{
					words++;
					space=0;
				}
			}
			else
			{
				if(str[i]==' ')
				{
					space=1;
				}	

			}
		}
		else
		{
			if(str[i]=='"')
			{
				isCommaOpen=0;
			}
		}
		
	}
	return words;
}

/*
This command will take a string(non-formatted,uglying spacing) and split it into tokens,fork and exec to run 
child.It will also check if the child succeeded in exec,therefore,it uses 'pipe' to print "error" from child 
to parent in cases\ of failure.
The split will consider spaces,that is any no.of spaces are allowed!(yes, I did that)
Also , it will not split a string argument
	Ex: cmd1 arg1 arg2 "arg3 blah blah" arg4
	Will be split as
		cmd1
		arg1
		arg2
		arg3 blah blah
		arg4
*/


int executeCommand(char *command,int shouldParentWait,char *filepath)
{
	//Return 0 on succes,-1 on 'exit'
	// This function will fork a new process to execute a command
	//We will first tokenise it
	/*int i=0;
	while(i<strlen(command[0])&&command[0][i]==' ')
	{
		i++;
	}*/
	int num_toks=wordcount(command);
	//printf("Expected Tokens:%d",num_toks);
	int isString=0;
	int i=0;
	char **tokens=NULL;
	tokens=(char **)malloc(sizeof(char *)*(num_toks+1));//+1 for NULL termination
	char *temp,*temp2;
	temp=strdup(command);
	while(temp!=NULL&&temp[0]==' ')
	{
		strsep(&temp," ");
	}
	while(i<num_toks)
	{
		temp2=strsep(&temp,"\"");
		while(isString==0&&temp2!=NULL)
		{
			tokens[i++]=strsep(&temp2," ");
			//printf("\nToken identifed:%s",tokens[i-1]);
			while(temp2!=NULL&&temp2[0]==' ')
			{
				strsep(&temp2," ");
			}
			if(temp2!=NULL&&strcmp(temp2,"")==0)
			{
				break;
			}
		}
		if(isString==1)
		{
			tokens[i++]=strdup(temp2);
			//printf("\nToken identifed:%s",tokens[i-1]);
			while(temp!=NULL&&temp[0]==' ')
			{
				strsep(&temp," ");
			}
		}
		isString=(isString==0)?1:0;

	}
	tokens[i]=NULL;
	
	free(command);
	if(strcmp(tokens[0],"cd")==0)//If cd,do chdir in parent
	{
		int try_dir_change=-1;
		char *path=NULL;
		if(num_toks>1)
		{
			path=strdup(tokens[1]);
			int i=2;
			while(i<num_toks)
			{
				strcat(path," ");
				strcat(path,tokens[i]);
				i++;
			}
			try_dir_change=chdir(path);

		}
		if(try_dir_change==-1)
		{
			//printf("Shell: Incorrect command\n");
			printf("Shell: Incorrect command\n");
		}
	}
	else if(strcmp(tokens[0],"exit")==0)//if exit,leave the shell
	{
		return -1;
	}
	else
	{
		//int pipefd[2];
		//pipe(pipefd);
		int child_pid=fork();
		if(child_pid==0)
		{
			if(filepath!=NULL)
			{
				//output redirection
				close(STDOUT_FILENO);
				open(filepath,O_CREAT|O_RDWR,S_IRWXU);
			}
			
			//child,restore default signals
			
			signal(SIGINT, SIG_DFL);
			signal(SIGTSTP, SIG_DFL);
			//close(pipefd[0]);//Read end
			if(execvp(tokens[0],tokens)==-1)
			{
				//Means error occured,mem image not overwritten
				//write(pipefd[1],"error",strlen("error"));
				printf("Shell: Incorrect command\n");
				//Using pipe to redirect op to parent ,if parent waits can be catched and error found!
			}
			exit(0);
			//DO some thing to find if exec fails
		}
		else if(child_pid>0)
		{
			//in parent;
			//close(pipefd[1]);//Write end
			int status;
			if(shouldParentWait==1)
			{
				waitpid(child_pid,&status,WUNTRACED);
				char isfail[10]={};
				/*read(pipefd[0],isfail,10);
				if(strlen(isfail)!=0)
				{
					//printf("Shell: Incorrect command\n");
				
					//printf("Shell: Incorrect command\n");
				}//*/
			}
		}	
		else
		{
			//fork failed
			printf("Shell: Incorrect command\n");
		}
	}
	return 0;
}

int executeParallelCommands(char **commands)
{
	// This function will run multiple commands in parallel
	int i=0,retval=0,no_of_child=0;
	while(commands[i]!=NULL)
	{
		retval=executeCommand(commands[i++],0,NULL);
		if(retval==-1)
		{
			break;
		}
		else
		{
			no_of_child++;
		}
	}
	while(no_of_child>0)
	{
		waitpid(-1,NULL,WUNTRACED);
		no_of_child--;
	}
	return retval;
}

int executeSequentialCommands(char **commands)
{	
	// This function will run multiple commands sequentially
	int i=0,retval=0;
	while(commands[i]!=NULL)
	{
		retval=executeCommand(commands[i++],1,NULL);
		if(retval==-1)
		{
			break;
		}
	}
	
	
	return retval;
}

int executeCommandRedirection(char *command)
{
	// This function will run a single command with output redirected to an output file specificed by user
	char *cmd,*filepath;
	int i=0,isCommaOpen=0,len=strlen(command),found=0;
	for(i=0;i<len&&found==0;i++)
	{
		if(isCommaOpen==0)
		{
			if(command[i]=='"')
			{
				isCommaOpen=1;
			}
			else if(command[i]=='>')
			{
				//split to cmd,filepath
				found=1;
				command[i]='\0';
				cmd=strdup(command);
				filepath=strdup((&command[i])+1);
				
				
				while(filepath!=NULL&&filepath[0]==' ')
				{
					strsep(&filepath," ");
				}
				
			}
		}
		else
		{
			if(command[i]='"')
			{
				isCommaOpen=0;
			}
		}
		
	}
	return executeCommand(cmd,1,filepath);
}

int main()
{
	// Initial declarations
	
	while(1)	// This loop will keep your shell running until user exits.
	{
		//Ignoring ctrl+c stopping
		signal(SIGINT,SIG_IGN);//Normal behaviour to be resumed in child
		signal(SIGTSTP,SIG_IGN);
		
		char *cwd=NULL,*command=NULL;
		size_t buffer_size=100,cmd_buffer=0;
		cwd=getcwd(cwd,buffer_size);
		// Print the prompt in format - currentWorkingDirectory$
		printf("%s$",cwd);
		// accept input with 'getline()'
		getline(&command,&cmd_buffer,stdin);//Remember to delloc the command
		
		command=strsep(&command,"\n");//Back '\n' trim
		
		
		// Parse input with 'strsep()' for different symbols (&&, ##, >) and for spaces.
		int mode;
		char **split_commands=NULL;//cmds here are not trimmed
		split_commands=parseInput(command,&mode);
		free(command);
		
		int shell_status=0;
		
		if(mode==1)
			shell_status=executeParallelCommands(split_commands);		// This function is invoked when user wants to run multiple commands in parallel (commands separated by &&)
		else if(mode==2)
			shell_status=executeSequentialCommands(split_commands);	// This function is invoked when user wants to run multiple commands sequentially (commands separated by ##)
		else if(mode==3)
			shell_status=executeCommandRedirection(split_commands[0]);	// This function is invoked when user wants redirect output of a single command to and output file specificed by user
		else if(mode==0)
			shell_status=executeCommand(split_commands[0],1,NULL);		// This function is invoked when user wants to run a single commands
			
		if(shell_status==-1)
		{
			printf("Exiting shell...\n");
			break;
		}
	}
	
	return 0;
}