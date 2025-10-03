# What's this?
catsh stands for C As Tangled SHell script.      
A tiny C library to execute shell commands with input/output redirection, implemented in pure C.      
As I like to use C, and for now I don't want to learn any other language, so I made this. At least C is more strict than shell script, that means less bugs (I hope so).           
Simple, stupid, paranoid, but I like it.       
It's the low-level implemention of rurima, and it's already used as part of rurima.      
# A simple demo:
```c
#include "include/catsh.h"
int main(void)
{
	char *stdin_msg = "Hello, catsh from stdin!\n";
	struct cth_result *res = cth_exec((char *[]){ "sh", "-c", "cat;echo Hello, catsh from stdout!; echo Hello, catsh from stderr! >&2; exit 0", NULL }, stdin_msg, true, true);
	printf("Demo: Execute a shell command with input and capture output\n");
	if (CTH_EXEC_SUCCEED(res)) {
		printf("  exit code = %d\n", res->exit_code);
		printf("  stdin(input):\n%s", stdin_msg);
		if (res->stdout_ret)
			printf("  stdout:\n%s", res->stdout_ret);
		else
			printf("  stdout: (null)\n");
		if (res->stderr_ret)
			printf("  stderr:\n%s", res->stderr_ret);
		else
			printf("  stderr: (null)\n");
		cth_free_result(&res);
	} else {
		if (CTH_EXEC_FAILED(res)) {
			printf("  Command exited with code %d\n", res->exit_code);
			cth_free_result(&res);
		} else if (CTH_EXEC_RUNNING(res)) {
			printf("  Command is still running\n");
			cth_free_result(&res);
		} else if (CTH_EXEC_CANNOT_RUN(res)) {
			printf("  Cannot run command\n");
		}
	}
	return 0;
}
```
Results:
```
Demo: Execute a shell command with input and capture output
  exit code = 0
  stdin(input):
Hello, catsh from stdin!
  stdout:
Hello, catsh from stdin!
Hello, catsh from stdout!
  stderr:
Hello, catsh from stderr!
```
# Benchmark results: 
Not very scientific, just a simple test.      
Tested on Ubuntu 24.04.3 LTS, OrbStack vm, MacBook Air M4.      
```
Performance Test: 128MB random string
  Command: cat
  Symmetric output: output equals input
Average elapsed time for 'cth_exec()': 0.030548 seconds
Average elapsed time for 'sh -c "x=$(cat temp_input.txt)"': 0.225729 seconds
Used -638.93% more time than shell variable $()

Performance Test: 1000x 'ls' command
  Command: ls >/dev/null (shell script vs cth_exec)
Average elapsed time for 1000x 'ls' in shell script: 0.236063 seconds
Average elapsed time for 1000x 'ls' via cth_exec(): 0.023837 seconds
Used -89.90% more time than shell script
```
Negative value means cth_exec() is faster.      
I think this is a good result, as catsh is only for small tasks.      