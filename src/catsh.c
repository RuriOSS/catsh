// SPDX-License-Identifier: MIT
/*
 *
 * This file is part of catsh, with ABSOLUTELY NO WARRANTY.
 *
 * MIT License
 *
 * Copyright (c) 2025 Moe-hacker
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 *
 */
#include "include/catsh.h"
int cth_add_arg(char ***argv, char *arg)
{
	/*
	 * Add an argument to the argv array.
	 * *argv: Pointer to the argv array. Can be NULL initially.
	 * arg: The argument to add, should be a null-terminated string.
	 * The argv array should be NULL-terminated.
	 * Returns 0 on success, -1 on failure.
	 * Warning: This function allocates memory.
	 * The caller is responsible for freeing it using cth_free_argv().
	 */
	size_t argc = 0;
	if (*argv != NULL) {
		while ((*argv)[argc] != NULL) {
			argc++;
		}
	}
	char **new_argv = realloc(*argv, sizeof(char *) * (argc + 2));
	if (new_argv == NULL) {
		return -1;
	}
	new_argv[argc] = strdup(arg);
	new_argv[argc + 1] = NULL;
	*argv = new_argv;
	return 0;
}
void cth_free_argv(char ***argv)
{
	/*
	 * Free the argv array and its contents.
	 * *argv: Pointer to the argv array, can be NULL.
	 * After calling this function, *argv will be set to NULL.
	 */
	if (*argv == NULL) {
		return;
	}
	size_t argc = 0;
	while ((*argv)[argc] != NULL) {
		free((*argv)[argc]);
		argc++;
	}
	free(*argv);
	*argv = NULL;
}
void cth_free_result(struct cth_result **res)
{
	/*
	 * Free the cth_result structure and its contents.
	 * *res: Pointer to the cth_result structure, can be NULL.
	 * After calling this function, *res will be set to NULL.
	 */
	if (*res == NULL) {
		return;
	}
	free((*res)->stdout_ret);
	free((*res)->stderr_ret);
	free(*res);
	*res = NULL;
}
void *cth_init_argv(void)
{
	/*
	 * Just a wrapper, returns NULL.
	 */
	return NULL;
}
static struct cth_result *cth_exec_nonblock(char **argv, char *input, bool get_output)
{
	// TODO
	return NULL;
}
static struct cth_result *cth_exec_block_without_stdio(char **argv)
{
	/*
	 * Just exec the command in blocking mode, without redirecting stdin/stdout/stderr.
	 * This is the simplest case.
	 */
	pid_t pid = fork();
	// Just error handling.
	if (pid < 0) {
		return NULL;
	}
	// Child process, exec the command.
	if (pid == 0) {
		int fd = open("/dev/null", O_RDWR);
		if (fd < 0) {
			exit(CTH_EXIT_FAILURE);
		}
		dup2(fd, STDIN_FILENO);
		dup2(fd, STDOUT_FILENO);
		dup2(fd, STDERR_FILENO);
		if (fd > 2) {
			close(fd);
		}
		execvp(argv[0], argv);
		exit(CTH_EXIT_FAILURE);
	}
	// Parent process, wait for child to exit.
	struct cth_result *res = malloc(sizeof(struct cth_result));
	if (res == NULL) {
		return NULL;
	}
	res->pid = pid;
	res->ppid = -1;
	res->exited = false;
	res->exit_code = -1;
	res->stdout_ret = NULL;
	res->stderr_ret = NULL;
	int status = 0;
	// Wait for child process, handle EINTR.
	while (waitpid(pid, &status, 0) < 0) {
		if (errno == EINTR) {
			continue;
		}
		free(res);
		return NULL;
	}
	// Get exit code.
	res->exited = true;
	if (WIFEXITED(status)) {
		res->exit_code = WEXITSTATUS(status);
	} else if (WIFSIGNALED(status)) {
		res->exit_code = 128 + WTERMSIG(status);
	} else {
		res->exit_code = -1;
	}
	return res;
}
static size_t pipe_buf_size(int fd)
{
	/*
	 * Get the pipe buffer size using fcntl.
	 * Returns the size on success, 0 on failure.
	 * Note: this function will also try to set the pipe buffer size to the maximum allowed size.
	 * As this is a internal function, we can do this.
	 */
	// Try to set the pipe buffer size to a large value.
	// Get max allowed size from /proc/sys/fs/pipe-max-size.
	FILE *f = fopen("/proc/sys/fs/pipe-max-size", "r");
	if (f) {
		char line[32];
		if (fgets(line, sizeof(line), f)) {
			size_t max_size = strtoul(line, NULL, 10);
			if (max_size > 0) {
				fcntl(fd, F_SETPIPE_SZ, max_size);
			}
		}
		fclose(f);
	}
	// Get the pipe buffer size using fcntl.
	int size = fcntl(fd, F_GETPIPE_SZ);
	if (size < 0) {
		return 0;
	}
	return (size_t)size;
}
static struct cth_result *cth_exec_block(char **argv, char *input, bool get_output)
{
	/*
	 * Exec the command in blocking mode, with optional stdin input and stdout/stderr capture.
	 * argv: The command and its arguments, NULL-terminated array of strings.
	 * input: The input to be passed to the command's stdin, can be NULL.
	 * get_output: If true, capture stdout and stderr output.
	 * Returns a cth_result structure on success, NULL on failure.
	 * The caller is responsible for freeing the result using cth_free_result().
	 */
	// For the simplest case, just exec without stdio redirection
	if (input == NULL && !get_output) {
		return cth_exec_block_without_stdio(argv);
	}
	// Create pipes for stdin, stdout, stderr.
	int stdin_pipe[2] = { -1, -1 };
	int stdout_pipe[2] = { -1, -1 };
	int stderr_pipe[2] = { -1, -1 };
	if (input != NULL) {
		if (pipe(stdin_pipe) < 0) {
			return NULL;
		}
		// Set write end of stdin pipe to non-blocking.
		int flags = fcntl(stdin_pipe[1], F_GETFL, 0);
		if (flags != -1) {
			fcntl(stdin_pipe[1], F_SETFL, flags | O_NONBLOCK);
		}
	}
	if (get_output) {
		if (pipe(stdout_pipe) < 0) {
			if (stdin_pipe[0] != -1) {
				close(stdin_pipe[0]);
				close(stdin_pipe[1]);
			}
			return NULL;
		}
		if (pipe(stderr_pipe) < 0) {
			if (stdin_pipe[0] != -1) {
				close(stdin_pipe[0]);
				close(stdin_pipe[1]);
			}
			if (stdout_pipe[0] != -1) {
				close(stdout_pipe[0]);
				close(stdout_pipe[1]);
			}
			return NULL;
		}
	}
	pid_t pid = fork();
	// Error handling.
	if (pid < 0) {
		if (stdin_pipe[0] != -1) {
			close(stdin_pipe[0]);
			close(stdin_pipe[1]);
		}
		if (stdout_pipe[0] != -1) {
			close(stdout_pipe[0]);
			close(stdout_pipe[1]);
		}
		if (stderr_pipe[0] != -1) {
			close(stderr_pipe[0]);
			close(stderr_pipe[1]);
		}
		return NULL;
	}
	if (pid == 0) {
		// Child process.
		if (input != NULL) {
			close(stdin_pipe[1]);
			dup2(stdin_pipe[0], STDIN_FILENO);
			close(stdin_pipe[0]);
		} else {
			int fd = open("/dev/null", O_RDONLY);
			if (fd >= 0) {
				dup2(fd, STDIN_FILENO);
				if (fd > 2) {
					close(fd);
				}
			}
		}
		if (get_output) {
			close(stdout_pipe[0]);
			dup2(stdout_pipe[1], STDOUT_FILENO);
			close(stdout_pipe[1]);
			close(stderr_pipe[0]);
			dup2(stderr_pipe[1], STDERR_FILENO);
			close(stderr_pipe[1]);
		} else {
			int fd = open("/dev/null", O_WRONLY);
			if (fd >= 0) {
				dup2(fd, STDOUT_FILENO);
				dup2(fd, STDERR_FILENO);
				if (fd > 2) {
					close(fd);
				}
			}
		}
		execvp(argv[0], argv);
		close(STDIN_FILENO);
		close(STDOUT_FILENO);
		close(STDERR_FILENO);
		_exit(CTH_EXIT_FAILURE);
	}
	// Parent process.
	// Close unused pipe ends.
	if (input != NULL) {
		close(stdin_pipe[0]);
	}
	if (get_output) {
		close(stdout_pipe[1]);
		close(stderr_pipe[1]);
	}
	struct cth_result *res = malloc(sizeof(struct cth_result));
	res->pid = pid;
	res->ppid = -1;
	res->exited = false;
	res->exit_code = -1;
	res->stdout_ret = NULL;
	res->stderr_ret = NULL;
	// Buffers for stdout and stderr.
	char *stdout_buf = NULL;
	char *stderr_buf = NULL;
	size_t stdout_size = 0;
	size_t stderr_size = 0;
	size_t stdout_cap = 0;
	size_t stderr_cap = 0;
	size_t BUF_CHUNK = 4096;
	if (get_output) {
		BUF_CHUNK = pipe_buf_size(stdout_pipe[0]);
		BUF_CHUNK = BUF_CHUNK > 0 ? BUF_CHUNK : 4096;
		size_t BUF_CHUNK_NEW = pipe_buf_size(stderr_pipe[0]);
		if (BUF_CHUNK_NEW > 0 && BUF_CHUNK_NEW < BUF_CHUNK) {
			BUF_CHUNK = BUF_CHUNK_NEW;
		}
	}
	if (input != NULL) {
		size_t BUF_CHUNK_NEW = pipe_buf_size(stdin_pipe[1]);
		if (BUF_CHUNK_NEW > 0 && BUF_CHUNK_NEW < BUF_CHUNK) {
			BUF_CHUNK = BUF_CHUNK_NEW;
		}
	}
	// poll loop to handle stdin, stdout, stderr.
	struct pollfd pfds[3];
	int nfds = 0;
	int stdin_idx = -1;
	int stdout_idx = -1;
	int stderr_idx = -1;
	if (input != NULL) {
		pfds[nfds].fd = stdin_pipe[1];
		pfds[nfds].events = POLLOUT;
		stdin_idx = nfds++;
	}
	if (get_output) {
		pfds[nfds].fd = stdout_pipe[0];
		pfds[nfds].events = POLLIN;
		stdout_idx = nfds++;
		pfds[nfds].fd = stderr_pipe[0];
		pfds[nfds].events = POLLIN;
		stderr_idx = nfds++;
	}
	ssize_t input_written = 0;
	ssize_t input_len = input ? (ssize_t)strlen(input) : 0;
	// Loop until all fds are closed.
	while (nfds > 0) {
		// Check if child exited before poll
		int status = 0;
		pid_t wait_ret = waitpid(pid, &status, WNOHANG);
		if (wait_ret == pid) {
			res->exited = true;
			if (WIFEXITED(status)) {
				res->exit_code = WEXITSTATUS(status);
			} else if (WIFSIGNALED(status)) {
				res->exit_code = 128 + WTERMSIG(status);
			} else {
				res->exit_code = -1;
			}
			// Continue reading stdout/stderr
			if (get_output && stdout_idx != -1) {
				ssize_t n;
				while ((n = read(stdout_pipe[0], stdout_buf ? stdout_buf + stdout_size : NULL, BUF_CHUNK)) > 0) {
					if (stdout_cap - stdout_size < (size_t)n) {
						stdout_cap = stdout_cap ? stdout_cap * 2 : BUF_CHUNK;
						stdout_buf = realloc(stdout_buf, stdout_cap);
					}
					memcpy(stdout_buf + stdout_size, stdout_buf + stdout_size, n);
					stdout_size += n;
				}
				close(stdout_pipe[0]);
				stdout_idx = -1;
			}
			if (get_output && stderr_idx != -1) {
				ssize_t n;
				while ((n = read(stderr_pipe[0], stderr_buf ? stderr_buf + stderr_size : NULL, BUF_CHUNK)) > 0) {
					if (stderr_cap - stderr_size < (size_t)n) {
						stderr_cap = stderr_cap ? stderr_cap * 2 : BUF_CHUNK;
						stderr_buf = realloc(stderr_buf, stderr_cap);
					}
					memcpy(stderr_buf + stderr_size, stderr_buf + stderr_size, n);
					stderr_size += n;
				}
				close(stderr_pipe[0]);
				stderr_idx = -1;
			}
			if (input != NULL && stdin_idx != -1) {
				close(stdin_pipe[1]);
				stdin_idx = -1;
			}
			// Set output buffers to result (if any data was read)
			if (get_output) {
				if (stdout_buf) {
					stdout_buf = realloc(stdout_buf, stdout_size + 1);
					stdout_buf[stdout_size] = 0;
					res->stdout_ret = stdout_buf;
				}
				if (stderr_buf) {
					stderr_buf = realloc(stderr_buf, stderr_size + 1);
					stderr_buf[stderr_size] = 0;
					res->stderr_ret = stderr_buf;
				}
			}
			return res;
		}
		int ret = poll(pfds, nfds, -1);
		if (ret < 0 && errno == EINTR) {
			continue;
		}
		if (ret < 0) {
			break;
		}
		// Write to stdin.
		if (input != NULL && stdin_idx != -1 && (pfds[stdin_idx].revents & POLLOUT)) {
			size_t remain = input_len - input_written;
			size_t chunk = remain > BUF_CHUNK ? BUF_CHUNK : remain;
			ssize_t n = write(stdin_pipe[1], input + input_written, chunk);
			if (n > 0) {
				input_written += n;
				if (input_written >= input_len) {
					close(stdin_pipe[1]);
					// remove stdin_idx from pfds.
					for (int i = stdin_idx + 1; i < nfds; ++i) {
						pfds[i - 1] = pfds[i];
					}
					nfds--;
					// update indexes.
					if (stdout_idx > stdin_idx) {
						stdout_idx--;
					}
					if (stderr_idx > stdin_idx) {
						stderr_idx--;
					}
					stdin_idx = -1;
				}
			} else if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
				// write buffer full, skip this round, wait for next poll.
				continue;
			} else {
				close(stdin_pipe[1]);
				// remove stdin_idx from pfds.
				for (int i = stdin_idx + 1; i < nfds; ++i) {
					pfds[i - 1] = pfds[i];
				}
				nfds--;
				if (stdout_idx > stdin_idx) {
					stdout_idx--;
				}
				if (stderr_idx > stdin_idx) {
					stderr_idx--;
				}
				stdin_idx = -1;
			}
		}
		// POLLHUP or POLLERR for stdin.
		if (input != NULL && stdin_idx != -1 && (pfds[stdin_idx].revents & (POLLHUP | POLLERR))) {
			close(stdin_pipe[1]);
			// remove stdin_idx from pfds.
			for (int i = stdin_idx + 1; i < nfds; ++i) {
				pfds[i - 1] = pfds[i];
			}
			nfds--;
			if (stdout_idx > stdin_idx) {
				stdout_idx--;
			}
			if (stderr_idx > stdin_idx) {
				stderr_idx--;
			}
			stdin_idx = -1;
		}
		// Read from stdout.
		if (get_output && stdout_idx != -1 && (pfds[stdout_idx].revents & POLLIN)) {
			if (stdout_cap - stdout_size < BUF_CHUNK) {
				stdout_cap = stdout_cap ? stdout_cap * 2 : BUF_CHUNK;
				stdout_buf = realloc(stdout_buf, stdout_cap);
			}
			ssize_t n = read(stdout_pipe[0], stdout_buf + stdout_size, BUF_CHUNK);
			if (n > 0) {
				stdout_size += n;
			} else {
				close(stdout_pipe[0]);
				// remove stdout_idx from pfds.
				for (int i = stdout_idx + 1; i < nfds; ++i) {
					pfds[i - 1] = pfds[i];
				}
				nfds--;
				if (stderr_idx > stdout_idx) {
					stderr_idx--;
				}
				stdout_idx = -1;
			}
		}
		// POLLHUP or POLLERR for stdout.
		if (get_output && stdout_idx != -1 && (pfds[stdout_idx].revents & (POLLHUP | POLLERR))) {
			close(stdout_pipe[0]);
			// remove stdout_idx from pfds.
			for (int i = stdout_idx + 1; i < nfds; ++i) {
				pfds[i - 1] = pfds[i];
			}
			nfds--;
			if (stderr_idx > stdout_idx) {
				stderr_idx--;
			}
			stdout_idx = -1;
		}
		// Read from stderr.
		if (get_output && stderr_idx != -1 && (pfds[stderr_idx].revents & POLLIN)) {
			if (stderr_cap - stderr_size < BUF_CHUNK) {
				stderr_cap = stderr_cap ? stderr_cap * 2 : BUF_CHUNK;
				stderr_buf = realloc(stderr_buf, stderr_cap);
			}
			ssize_t n = read(stderr_pipe[0], stderr_buf + stderr_size, BUF_CHUNK);
			if (n > 0) {
				stderr_size += n;
			} else {
				close(stderr_pipe[0]);
				// remove stderr_idx from pfds.
				for (int i = stderr_idx + 1; i < nfds; ++i) {
					pfds[i - 1] = pfds[i];
				}
				nfds--;
				stderr_idx = -1;
			}
		}
		// POLLHUP or POLLERR for stderr.
		if (get_output && stderr_idx != -1 && (pfds[stderr_idx].revents & (POLLHUP | POLLERR))) {
			close(stderr_pipe[0]);
			// remove stderr_idx from pfds.
			for (int i = stderr_idx + 1; i < nfds; ++i) {
				pfds[i - 1] = pfds[i];
			}
			nfds--;
			stderr_idx = -1;
		}
		// Check if all fds are closed.
		if ((stdin_idx == -1) && (stdout_idx == -1) && (stderr_idx == -1)) {
			break;
		}
	}
	// Parent process, wait for child to exit.
	int status = 0;
	// Wait for child process, handle EINTR
	while (waitpid(pid, &status, 0) < 0) {
		if (errno == EINTR) {
			continue;
		}
		break;
	}
	res->exited = true;
	if (WIFEXITED(status)) {
		res->exit_code = WEXITSTATUS(status);
	} else if (WIFSIGNALED(status)) {
		res->exit_code = 128 + WTERMSIG(status);
	} else {
		res->exit_code = -1;
	}
	// Set output buffers to result.
	if (get_output) {
		// Make sure buffers are null-terminated.
		if (stdout_buf) {
			stdout_buf = realloc(stdout_buf, stdout_size + 1);
			stdout_buf[stdout_size] = 0;
			res->stdout_ret = stdout_buf;
		}
		if (stderr_buf) {
			stderr_buf = realloc(stderr_buf, stderr_size + 1);
			stderr_buf[stderr_size] = 0;
			res->stderr_ret = stderr_buf;
		}
	}
	return res;
}
// API function.
struct cth_result *cth_exec(char **argv, char *input, bool block, bool get_output)
{
	/*
	 * Exec the command with given arguments.
	 * argv: The command and its arguments, NULL-terminated array of strings.
	 * input: The input to be passed to the command's stdin, can be NULL.
	 * block: If true, wait for the command to finish and return the result.
	 *        If false, return immediately (not implemented yet).
	 * get_output: If true, capture stdout and stderr output.
	 * Returns a cth_result structure on success, NULL on failure.
	 * The caller is responsible for freeing the result using cth_free_result().
	 */
	if (argv == NULL || argv[0] == NULL) {
		return NULL;
	}
	// For now, only blocking mode is implemented.
	if (block) {
		return cth_exec_block(argv, input, get_output);
	}
	// TODO
	cth_debug(printf("cth_exec: non-blocking mode not implemented yet\n"););
	return NULL;
}
// API function.
int cth_exec_command(char **argv)
{
	/*
	 * Just exec the command in blocking mode, and return the exit code.
	 * If the command cannot be executed, return -1.
	 * This is a simple wrapper around cth_exec().
	 */
	struct cth_result *res = cth_exec(argv, NULL, true, false);
	if (res == NULL) {
		return -1;
	}
	int exit_code = res->exit_code;
	cth_free_result(&res);
	return exit_code;
}
int cth_wait(struct cth_result **res)
{
	// TODO
	return -1;
}
int cth_fork_rexec_self(char *const argv[])
{
	/*
	 * Fork and re-exec the current executable with given arguments.
	 * argv: The arguments to pass to the new executable, NULL-terminated array of strings.
	 * Returns the exit code of the new process on success, -1 on failure.
	 * Note: This function will block, and use current terminal for stdio.
	 */
	pid_t pid = fork();
	if (pid == -1) {
		return -1;
	}
	if (pid == 0) {
		size_t argc = 0;
		while (argv[argc] != NULL) {
			argc++;
		}
		char **new_argv = (char **)malloc(sizeof(char *) * (argc + 2));
		new_argv[0] = "/proc/self/exe";
		for (size_t i = 0; i < argc; i++) {
			new_argv[i + 1] = argv[i];
		}
		new_argv[argc + 1] = NULL;
		execv(new_argv[0], new_argv);
		free(new_argv);
		_exit(CTH_EXIT_FAILURE);
	}
	int status = 0;
	waitpid(pid, &status, 0);
	return WEXITSTATUS(status);
}
static struct cth_result *cth_exec_block_with_file_input(char **argv, int input_fd, bool get_output, void (*progress)(float, int), int progress_line_num)
{
	/*
	 * Exec the command in blocking mode, with file descriptor input and optional stdout/stderr capture.
	 * argv: The command and its arguments, NULL-terminated array of strings.
	 * fd: The file descriptor to read input from, should be readable.
	 * get_output: If true, capture stdout and stderr output.
	 * progress: A callback function to report progress, can be NULL.
	 */
	if (input_fd < 0) {
		return NULL;
	}
	// Create pipes for stdin, stdout, stderr.
	int stdin_pipe[2] = { -1, -1 };
	int stdout_pipe[2] = { -1, -1 };
	int stderr_pipe[2] = { -1, -1 };
	if (pipe(stdin_pipe) < 0) {
		return NULL;
	}
	// Set write end of stdin pipe to non-blocking.
	int flags = fcntl(stdin_pipe[1], F_GETFL, 0);
	if (flags != -1) {
		fcntl(stdin_pipe[1], F_SETFL, flags | O_NONBLOCK);
	}
	if (get_output) {
		if (pipe(stdout_pipe) < 0) {
			if (stdin_pipe[0] != -1) {
				close(stdin_pipe[0]);
				close(stdin_pipe[1]);
			}
			return NULL;
		}
		if (pipe(stderr_pipe) < 0) {
			if (stdin_pipe[0] != -1) {
				close(stdin_pipe[0]);
				close(stdin_pipe[1]);
			}
			if (stdout_pipe[0] != -1) {
				close(stdout_pipe[0]);
				close(stdout_pipe[1]);
			}
			return NULL;
		}
	}
	pid_t pid = fork();
	// Error handling.
	if (pid < 0) {
		if (stdin_pipe[0] != -1) {
			close(stdin_pipe[0]);
			close(stdin_pipe[1]);
		}
		if (stdout_pipe[0] != -1) {
			close(stdout_pipe[0]);
			close(stdout_pipe[1]);
		}
		if (stderr_pipe[0] != -1) {
			close(stderr_pipe[0]);
			close(stderr_pipe[1]);
		}
		return NULL;
	}
	if (pid == 0) {
		// Child process.
		close(stdin_pipe[1]);
		dup2(stdin_pipe[0], STDIN_FILENO);
		close(stdin_pipe[0]);
		if (get_output) {
			close(stdout_pipe[0]);
			dup2(stdout_pipe[1], STDOUT_FILENO);
			close(stdout_pipe[1]);
			close(stderr_pipe[0]);
			dup2(stderr_pipe[1], STDERR_FILENO);
			close(stderr_pipe[1]);
		} else {
			int fd = open("/dev/null", O_WRONLY);
			if (fd >= 0) {
				dup2(fd, STDOUT_FILENO);
				dup2(fd, STDERR_FILENO);
				if (fd > 2) {
					close(fd);
				}
			}
		}
		execvp(argv[0], argv);
		close(STDIN_FILENO);
		close(STDOUT_FILENO);
		close(STDERR_FILENO);
		_exit(CTH_EXIT_FAILURE);
	}
	// Parent process.
	if (get_output) {
		close(stdout_pipe[1]);
		close(stderr_pipe[1]);
	}
	struct cth_result *res = malloc(sizeof(struct cth_result));
	res->pid = pid;
	res->ppid = -1;
	res->exited = false;
	res->exit_code = -1;
	res->stdout_ret = NULL;
	res->stderr_ret = NULL;
	// Buffers for stdout and stderr.
	char *stdout_buf = NULL;
	char *stderr_buf = NULL;
	size_t stdout_size = 0;
	size_t stderr_size = 0;
	size_t stdout_cap = 0;
	size_t stderr_cap = 0;
	size_t BUF_CHUNK = 1024;
	char input_buf[BUF_CHUNK + 1];
	// poll loop to handle stdin, stdout, stderr.
	struct pollfd pfds[3];
	int nfds = 0;
	int stdin_idx = -1;
	int stdout_idx = -1;
	int stderr_idx = -1;
	pfds[nfds].fd = stdin_pipe[1];
	pfds[nfds].events = POLLOUT;
	stdin_idx = nfds++;
	if (get_output) {
		pfds[nfds].fd = stdout_pipe[0];
		pfds[nfds].events = POLLIN;
		stdout_idx = nfds++;
		pfds[nfds].fd = stderr_pipe[0];
		pfds[nfds].events = POLLIN;
		stderr_idx = nfds++;
	}
	ssize_t input_written = 0;
	ssize_t input_len = 0;
	struct stat st;
	if (fstat(input_fd, &st) == 0) {
		if (S_ISREG(st.st_mode) || S_ISFIFO(st.st_mode)) {
			input_len = st.st_size;
		}
	}
	// Loop until all fds are closed.
	while (nfds > 0) {
		// Check if child exited before poll
		int status = 0;
		pid_t wait_ret = waitpid(pid, &status, WNOHANG);
		if (wait_ret == pid) {
			res->exited = true;
			if (WIFEXITED(status)) {
				res->exit_code = WEXITSTATUS(status);
			} else if (WIFSIGNALED(status)) {
				res->exit_code = 128 + WTERMSIG(status);
			} else {
				res->exit_code = -1;
			}
			// Continue reading stdout/stderr
			if (get_output && stdout_idx != -1) {
				ssize_t n;
				while ((n = read(stdout_pipe[0], stdout_buf ? stdout_buf + stdout_size : NULL, BUF_CHUNK)) > 0) {
					if (stdout_cap - stdout_size < (size_t)n) {
						stdout_cap = stdout_cap ? stdout_cap * 2 : BUF_CHUNK;
						stdout_buf = realloc(stdout_buf, stdout_cap);
					}
					memcpy(stdout_buf + stdout_size, stdout_buf + stdout_size, n);
					stdout_size += n;
				}
				close(stdout_pipe[0]);
				stdout_idx = -1;
			}
			if (get_output && stderr_idx != -1) {
				ssize_t n;
				while ((n = read(stderr_pipe[0], stderr_buf ? stderr_buf + stderr_size : NULL, BUF_CHUNK)) > 0) {
					if (stderr_cap - stderr_size < (size_t)n) {
						stderr_cap = stderr_cap ? stderr_cap * 2 : BUF_CHUNK;
						stderr_buf = realloc(stderr_buf, stderr_cap);
					}
					memcpy(stderr_buf + stderr_size, stderr_buf + stderr_size, n);
					stderr_size += n;
				}
				close(stderr_pipe[0]);
				stderr_idx = -1;
			}
			if (stdin_idx != -1) {
				close(stdin_pipe[1]);
				stdin_idx = -1;
			}
			// Set output buffers to result (if any data was read)
			if (get_output) {
				if (stdout_buf) {
					stdout_buf = realloc(stdout_buf, stdout_size + 1);
					stdout_buf[stdout_size] = 0;
					res->stdout_ret = stdout_buf;
				}
				if (stderr_buf) {
					stderr_buf = realloc(stderr_buf, stderr_size + 1);
					stderr_buf[stderr_size] = 0;
					res->stderr_ret = stderr_buf;
				}
			}
			return res;
		}
		int ret = poll(pfds, nfds, -1);
		if (ret < 0 && errno == EINTR) {
			continue;
		}
		if (ret < 0) {
			break;
		}
		if (progress != NULL) {
			progress((float)input_written / (float)(input_len ? input_len : 1), progress_line_num);
		}
		// Write to stdin.
		if (stdin_idx != -1 && (pfds[stdin_idx].revents & POLLOUT)) {
			ssize_t r = read(input_fd, input_buf, BUF_CHUNK);
			if (r > 0) {
				ssize_t n = write(stdin_pipe[1], input_buf, r);
				if (n > 0) {
					input_written += n;
				} else if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
					continue;
				} else {
					close(stdin_pipe[1]);
					for (int i = stdin_idx + 1; i < nfds; ++i) {
						pfds[i - 1] = pfds[i];
					}
					nfds--;
					if (stdout_idx > stdin_idx) {
						stdout_idx--;
					}
					if (stderr_idx > stdin_idx) {
						stderr_idx--;
					}
					stdin_idx = -1;
				}
			} else if (r == 0 || (r < 0 && errno != EAGAIN && errno != EWOULDBLOCK)) {
				// EOF or read error (not EAGAIN): close stdin_pipe[1] and remove from poll
				close(stdin_pipe[1]);
				for (int i = stdin_idx + 1; i < nfds; ++i) {
					pfds[i - 1] = pfds[i];
				}
				nfds--;
				if (stdout_idx > stdin_idx) {
					stdout_idx--;
				}
				if (stderr_idx > stdin_idx) {
					stderr_idx--;
				}
				stdin_idx = -1;
			}
		}
		// POLLHUP or POLLERR for stdin.
		if (stdin_idx != -1 && (pfds[stdin_idx].revents & (POLLHUP | POLLERR))) {
			close(stdin_pipe[1]);
			// remove stdin_idx from pfds.
			for (int i = stdin_idx + 1; i < nfds; ++i) {
				pfds[i - 1] = pfds[i];
			}
			nfds--;
			if (stdout_idx > stdin_idx) {
				stdout_idx--;
			}
			if (stderr_idx > stdin_idx) {
				stderr_idx--;
			}
			stdin_idx = -1;
		}
		// Read from stdout.
		if (get_output && stdout_idx != -1 && (pfds[stdout_idx].revents & POLLIN)) {
			if (stdout_cap - stdout_size < BUF_CHUNK) {
				stdout_cap = stdout_cap ? stdout_cap * 2 : BUF_CHUNK;
				stdout_buf = realloc(stdout_buf, stdout_cap);
			}
			ssize_t n = read(stdout_pipe[0], stdout_buf + stdout_size, BUF_CHUNK);
			if (n > 0) {
				stdout_size += n;
			} else {
				close(stdout_pipe[0]);
				// remove stdout_idx from pfds.
				for (int i = stdout_idx + 1; i < nfds; ++i) {
					pfds[i - 1] = pfds[i];
				}
				nfds--;
				if (stderr_idx > stdout_idx) {
					stderr_idx--;
				}
				stdout_idx = -1;
			}
		}
		// POLLHUP or POLLERR for stdout.
		if (get_output && stdout_idx != -1 && (pfds[stdout_idx].revents & (POLLHUP | POLLERR))) {
			close(stdout_pipe[0]);
			// remove stdout_idx from pfds.
			for (int i = stdout_idx + 1; i < nfds; ++i) {
				pfds[i - 1] = pfds[i];
			}
			nfds--;
			if (stderr_idx > stdout_idx) {
				stderr_idx--;
			}
			stdout_idx = -1;
		}
		// Read from stderr.
		if (get_output && stderr_idx != -1 && (pfds[stderr_idx].revents & POLLIN)) {
			if (stderr_cap - stderr_size < BUF_CHUNK) {
				stderr_cap = stderr_cap ? stderr_cap * 2 : BUF_CHUNK;
				stderr_buf = realloc(stderr_buf, stderr_cap);
			}
			ssize_t n = read(stderr_pipe[0], stderr_buf + stderr_size, BUF_CHUNK);
			if (n > 0) {
				stderr_size += n;
			} else {
				close(stderr_pipe[0]);
				// remove stderr_idx from pfds.
				for (int i = stderr_idx + 1; i < nfds; ++i) {
					pfds[i - 1] = pfds[i];
				}
				nfds--;
				stderr_idx = -1;
			}
		}
		// POLLHUP or POLLERR for stderr.
		if (get_output && stderr_idx != -1 && (pfds[stderr_idx].revents & (POLLHUP | POLLERR))) {
			close(stderr_pipe[0]);
			// remove stderr_idx from pfds.
			for (int i = stderr_idx + 1; i < nfds; ++i) {
				pfds[i - 1] = pfds[i];
			}
			nfds--;
			stderr_idx = -1;
		}
		// Check if all fds are closed.
		if ((stdin_idx == -1) && (stdout_idx == -1) && (stderr_idx == -1)) {
			break;
		}
	}
	// Parent process, wait for child to exit.
	int status = 0;
	// Wait for child process, handle EINTR
	while (waitpid(pid, &status, 0) < 0) {
		if (errno == EINTR) {
			continue;
		}
		break;
	}
	res->exited = true;
	if (WIFEXITED(status)) {
		res->exit_code = WEXITSTATUS(status);
	} else if (WIFSIGNALED(status)) {
		res->exit_code = 128 + WTERMSIG(status);
	} else {
		res->exit_code = -1;
	}
	// Set output buffers to result.
	if (get_output) {
		// Make sure buffers are null-terminated.
		if (stdout_buf) {
			stdout_buf = realloc(stdout_buf, stdout_size + 1);
			stdout_buf[stdout_size] = 0;
			res->stdout_ret = stdout_buf;
		}
		if (stderr_buf) {
			stderr_buf = realloc(stderr_buf, stderr_size + 1);
			stderr_buf[stderr_size] = 0;
			res->stderr_ret = stderr_buf;
		}
	}
	progress(-1.0f, progress_line_num);
	return res;
}
// API function.
struct cth_result *cth_exec_with_file_input(char **argv, int fd, bool block, bool get_output, void (*progress)(float, int), int progress_line_num)
{
	/*
	 * Exec the command with given arguments, using the given file descriptor as stdin.
	 * argv: The command and its arguments, NULL-terminated array of strings.
	 * fd: The file descriptor to use as stdin, should be valid and open for reading.
	 * block: If true, wait for the command to finish and return the result.
	 *        If false, return immediately (not implemented yet).
	 * get_output: If true, capture stdout and stderr output.
	 * progress: A callback function to report progress, can be NULL.
	 *           The function will be called with a float value between 0.0 and 1.0,
	 *           representing the progress of reading the input file, and an integer
	 *           line number to indicate where to print the progress (for multi-line progress).
	 * progress_line_num: The line number to use for progress reporting, if progress is not NULL.
	 * Returns a cth_result structure on success, NULL on failure.
	 * The caller is responsible for freeing the result using cth_free_result().
	 */
	if (argv == NULL || argv[0] == NULL) {
		return NULL;
	}
	// For now, only blocking mode is implemented.
	if (block) {
		return cth_exec_block_with_file_input(argv, fd, get_output, progress, progress_line_num);
	}
	// TODO
	cth_debug(printf("cth_exec_with_file_input: non-blocking mode not implemented yet\n"););
	return NULL;
}
void cth_show_progress(float progress, int line_num)
{
	/*
	 * This is an example progress reporting function.
	 * Show a progress bar in the terminal.
	 * progress: A float value between 0.0 and 1.0, representing the progress.
	 *           If progress < 0.0, clear the progress bar.
	 *           If progress > 1.0, treat as 1.0.
	 * line_num: The line number to use for progress reporting, if > 0.
	 *           If line_num <= 0, use the current line.
	 * Note: This function uses ANSI escape codes to move the cursor.
	 */
	if (progress < 0.0f) {
		printf("\n");
		fflush(stdout);
		return;
	}
	if (progress > 1.0f) {
		progress = 1.0f;
	}
	const int bar_width = 50;
	int pos = (int)(bar_width * progress);
	// Move cursor to the specified line.
	if (line_num > 0) {
		printf("\033[%dA", line_num);
	}
	printf("[");
	for (int i = 0; i < bar_width; ++i) {
		if (i < pos) {
			printf("=");
		} else if (i == pos) {
			printf(">");
		} else {
			printf(" ");
		}
	}
	printf("] %3d %%\r", (int)(progress * 100.0f));
	fflush(stdout);
	// Move cursor back to original position.
	if (line_num > 0) {
		printf("\033[%dB", line_num);
	}
}