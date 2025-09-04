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
		while ((*argv)[argc] != NULL)
			argc++;
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
	int status;
	// Wait for child process, handle EINTR.
	while (waitpid(pid, &status, 0) < 0) {
		if (errno == EINTR)
			continue;
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
		if (pipe(stdin_pipe) < 0)
			return NULL;
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
	if (input != NULL)
		close(stdin_pipe[0]);
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
	char *stdout_buf = NULL, *stderr_buf = NULL;
	size_t stdout_size = 0, stderr_size = 0;
	size_t stdout_cap = 0, stderr_cap = 0;
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
	int stdin_idx = -1, stdout_idx = -1, stderr_idx = -1;
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
	ssize_t input_len = input ? strlen(input) : 0;
	int stdin_closed = (input == NULL);
	// Loop until all fds are closed.
	while (nfds > 0) {
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
		if ((stdin_idx == -1) && (stdout_idx == -1) && (stderr_idx == -1))
			break;
	}
	// Parent process, wait for child to exit.
	int status;
	// Wait for child process, handle EINTR
	while (waitpid(pid, &status, 0) < 0) {
		if (errno == EINTR) {
			continue;
		} else {
			break;
		}
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