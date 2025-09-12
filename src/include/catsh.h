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
#ifndef __linux__
#error "This code is intended to be compiled on Linux systems only."
#endif
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <poll.h>
#include <sys/stat.h>
// Bool!!!
#if __STDC_VERSION__ < 202000L
#ifndef bool
#define bool _Bool
#define true ((_Bool) + 1u)
#define false ((_Bool) + 0u)
#endif
#endif
#define cth_debug(x) \
	do {         \
		x    \
	} while (0)
#define cth_log(format, ...)                                                                                                  \
	{                                                                                                                     \
		struct timeval tv;                                                                                            \
		gettimeofday(&tv, NULL);                                                                                      \
		fprintf(stderr, "[%ld.%06ld] in %s() in %s line %d:\n", tv.tv_sec, tv.tv_usec, __func__, __FILE__, __LINE__); \
		fprintf(stderr, format, ##__VA_ARGS__);                                                                       \
	}
#define CTH_EXIT_FAILURE 114
#define CTH_EXIT_SUCCESS 0
#define CTH_VERSION_MAJOR 0
#define CTH_VERSION_MINOR 5
#define CTH_VERSION_PATCH 0
#define CTH_VERSION_STRING "0.5.0"
struct cth_result {
	bool exited;
	int exit_code;
	char *stdout_ret;
	char *stderr_ret;
	pid_t pid;
	pid_t ppid;
};
int cth_add_arg(char ***argv, char *arg);
void cth_free_argv(char ***argv);
void cth_free_result(struct cth_result **res);
struct cth_result *cth_exec(char **argv, char *input, bool block, bool get_output);
int cth_fork_rexec_self(char *const argv[]);
int cth_exec_command(char **argv);
// TODO: non-blocking exec
int cth_wait(struct cth_result **res);
void *cth_init_argv(void);
struct cth_result *cth_exec_with_file_input(char **argv, int fd, bool block, bool get_output, void (*progress)(float, int), int progress_line_num);
void cth_show_progress(float progress, int line_num);