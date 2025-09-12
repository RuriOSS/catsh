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
int main(void)
{
	char *stdin_msg = "Hello, catsh from stdin!\n";
	struct cth_result *res = cth_exec((char *[]){ "sh", "-c", "cat;echo Hello, catsh from stdout!; echo Hello, catsh from stderr! >&2; exit 0", NULL }, stdin_msg, true, true);
	printf("Demo: Execute a shell command with input and capture output\n");
	if (res != NULL) {
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
		printf("  cth_exec failed\n");
	}
	mkdir("test_e", 0755);
	struct cth_result *res2 = cth_exec_with_file_input((char *[]){ "tar", "-xJf", "-", "-C", "./test_e", NULL }, open("rootfs.tar.xz", O_RDONLY), true, true, cth_show_progress, 0);
	printf("Demo: Extract a tar file using catsh\n");
	if (res2 != NULL) {
		printf("  exit code = %d\n", res2->exit_code);
		if (res2->stdout_ret)
			printf("  stdout:\n%s", res2->stdout_ret);
		else
			printf("  stdout: (null)\n");
		if (res2->stderr_ret)
			printf("  stderr:\n%s", res2->stderr_ret);
		else
			printf("  stderr: (null)\n");
		cth_free_result(&res2);
	} else {
		printf("  cth_exec_with_file_input failed\n");
	}
	return 0;
}