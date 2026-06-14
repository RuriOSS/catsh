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
#include "../src/include/catsh.h"
void t1()
{
	char *argv[] = { "sh", "-c", "cat;echo hello; sleep 1; echo world >&2; sleep 1; echo done", NULL };
	struct cth_result *res = cth_exec(argv, "hvbwhoowhveborbfo\n", false, true);
	if (cth_wait(&res) > 0) {
		printf("Exit code: %d\n", res->exit_code);
		printf("Stdout: %s\n", res->stdout_ret ? res->stdout_ret : "(null)");
		printf("Stderr: %s\n", res->stderr_ret ? res->stderr_ret : "(null)");
	} else {
		while (cth_wait(&res) < 0) {
			printf("Waiting for process to exit...\n");
			if (CTH_EXEC_RUNNING(res)) {
				printf("Process is still running...\n");
			} else {
				printf("Process is not running, but not exited yet...\n");
			}
			sleep(1);
		}
		if (CTH_EXEC_SUCCEED(res)) {
			printf("Process exited successfully.\n");
		} else if (CTH_EXEC_FAILED(res)) {
			printf("Process exited with failure.\n");
		} else {
			printf("Process exited with unknown status.\n");
		}
		printf("Exit code: %d\n", res->exit_code);
		printf("Stdout: %s\n", res->stdout_ret ? res->stdout_ret : "(null)");
		printf("Stderr: %s\n", res->stderr_ret ? res->stderr_ret : "(null)");
	}
	cth_free_result(&res);
}
void t2()
{
	char *argv[] = { "sh", "-c", "cat >&2;echo hello; sleep 1; echo world >&2; sleep 1; echo done;exit 1", NULL };
	struct cth_result *res = cth_exec(argv, "hbvjbvwhkwbjf\n", false, true);
	if (cth_wait(&res) > 0) {
		printf("Exit code: %d\n", res->exit_code);
		printf("Stdout: %s\n", res->stdout_ret ? res->stdout_ret : "(null)");
		printf("Stderr: %s\n", res->stderr_ret ? res->stderr_ret : "(null)");
	} else {
		while (cth_wait(&res) < 0) {
			printf("Waiting for process to exit...\n");
			if (CTH_EXEC_RUNNING(res)) {
				printf("Process is still running...\n");
			} else {
				printf("Process is not running, but not exited yet...\n");
			}
			sleep(1);
		}
		if (CTH_EXEC_SUCCEED(res)) {
			printf("Process exited successfully.\n");
		} else if (CTH_EXEC_FAILED(res)) {
			printf("Process exited with failure.\n");
		} else {
			printf("Process exited with unknown status.\n");
		}
		printf("Exit code: %d\n", res->exit_code);
		printf("Stdout: %s\n", res->stdout_ret ? res->stdout_ret : "(null)");
		printf("Stderr: %s\n", res->stderr_ret ? res->stderr_ret : "(null)");
	}
	cth_free_result(&res);
}
int main()
{
	t1();
	t2();
}