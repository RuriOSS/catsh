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
#include <time.h>
#include <stdlib.h>
#define PERF_SIZE (128 * 1024 * 1024)
void perf_test_1()
{
	printf("\nPerformance Test: 128MB random string\n");
	printf("  Command: ./test_cat_1\n");
	printf("  Asymmetry output: each input chunk is prefixed by 114 'A's\n");
	char *bigstr = malloc(PERF_SIZE + 1);
	if (!bigstr) {
		printf("  Failed to allocate memory\n");
		return;
	}
	for (size_t i = 0; i < PERF_SIZE; ++i) {
		bigstr[i] = 'a' + rand() % 26;
	}
	bigstr[PERF_SIZE - 1] = '0';
	bigstr[PERF_SIZE] = 0;

	struct timespec ts1, ts2;
	clock_gettime(CLOCK_MONOTONIC, &ts1);
	struct cth_result *res = cth_exec((char *[]){ "./test_cat_1", NULL }, bigstr, true, true);
	clock_gettime(CLOCK_MONOTONIC, &ts2);

	double elapsed = (ts2.tv_sec - ts1.tv_sec) + (ts2.tv_nsec - ts1.tv_nsec) / 1e9;
	if (res) {
		printf("  exit code: %d\n", res->exit_code);
		printf("  stdout size: %.2f MB\n", res->stdout_ret ? strlen(res->stdout_ret) / (1024.0 * 1024.0) : 0.0);
		printf("  elapsed: %.6f seconds\n", elapsed);
		cth_free_result(&res);
	} else {
		printf("  cth_exec failed\n");
	}
	free(bigstr);
}
void perf_test_2()
{
	printf("\nPerformance Test: 128MB random string\n");
	printf("  Command: ./test_cat_2\n");
	printf("  Asymmetry output: each input chunk is halved\n");
	char *bigstr = malloc(PERF_SIZE + 1);
	if (!bigstr) {
		printf("  Failed to allocate memory\n");
		return;
	}
	for (size_t i = 0; i < PERF_SIZE; ++i) {
		bigstr[i] = 'a' + rand() % 26;
	}
	bigstr[PERF_SIZE - 1] = '0';
	bigstr[PERF_SIZE] = 0;

	struct timespec ts1, ts2;
	clock_gettime(CLOCK_MONOTONIC, &ts1);
	struct cth_result *res = cth_exec((char *[]){ "./test_cat_2", NULL }, bigstr, true, true);
	clock_gettime(CLOCK_MONOTONIC, &ts2);

	double elapsed = (ts2.tv_sec - ts1.tv_sec) + (ts2.tv_nsec - ts1.tv_nsec) / 1e9;
	if (res) {
		printf("  exit code: %d\n", res->exit_code);
		printf("  stdout size: %.2f MB\n", res->stdout_ret ? strlen(res->stdout_ret) / (1024.0 * 1024.0) : 0.0);
		printf("  elapsed: %.6f seconds\n", elapsed);
		cth_free_result(&res);
	} else {
		printf("  cth_exec failed\n");
	}
	free(bigstr);
}
void perf_test_3()
{
	printf("\nPerformance Test: 128MB random string\n");
	printf("  Command: cat\n");
	printf("  Symmetric output: output equals input\n");
	char *bigstr = malloc(PERF_SIZE + 1);
	if (!bigstr) {
		printf("  Failed to allocate memory\n");
		return;
	}
	for (size_t i = 0; i < PERF_SIZE; ++i) {
		bigstr[i] = 'a' + rand() % 26;
	}
	bigstr[PERF_SIZE - 1] = '0';
	bigstr[PERF_SIZE] = 0;
	// Write bigstr to a temporary file
	FILE *f = fopen("temp_input.txt", "w");
	if (!f) {
		printf("  Failed to open temp_input.txt for writing\n");
		free(bigstr);
		return;
	}
	fwrite(bigstr, 1, PERF_SIZE, f);
	fclose(f);
	double total_elapsed_cat = 0.0, total_elapsed_shcat = 0.0, total_elapsed_sh = 0.0;
	for (int i = 0; i < 100; ++i) {
		struct timespec ts1, ts2;
		clock_gettime(CLOCK_MONOTONIC, &ts1);
		struct cth_result *res = cth_exec((char *[]){ "cat", NULL }, bigstr, true, true);
		clock_gettime(CLOCK_MONOTONIC, &ts2);
		double elapsed = (ts2.tv_sec - ts1.tv_sec) + (ts2.tv_nsec - ts1.tv_nsec) / 1e9;
		total_elapsed_cat += elapsed;
		if (res) {
			cth_free_result(&res);
		}
	}
	for (int i = 0; i < 100; ++i) {
		struct timespec ts1, ts2;
		clock_gettime(CLOCK_MONOTONIC, &ts1);
		int status = cth_exec_command((char *[]){ "sh", "-c", "x=$(cat temp_input.txt)", NULL });
		clock_gettime(CLOCK_MONOTONIC, &ts2);
		double elapsed = (ts2.tv_sec - ts1.tv_sec) + (ts2.tv_nsec - ts1.tv_nsec) / 1e9;
		total_elapsed_sh += elapsed;
	}
	printf("Average elapsed time for 'cth_exec()': %.6f seconds\n", total_elapsed_cat / 100.0);
	printf("Average elapsed time for 'sh -c \"x=$(cat temp_input.txt)\"': %.6f seconds\n", total_elapsed_sh / 100.0);
	// Calculate and print the percent difference between the two averages
	if (total_elapsed_cat > 0) {
		double percent_diff = ((total_elapsed_cat - total_elapsed_sh) / total_elapsed_cat) * 100.0;
		printf("Used %.2f%% more time than shell variable $()\n", percent_diff);
	}
	free(bigstr);
	remove("temp_input.txt");
}
void perf_test_4()
{
	printf("\nPerformance Test: 1000x 'ls' command\n");
	printf("  Command: ls >/dev/null (shell script vs cth_exec)\n");
	// Write the shell script if not exists
	FILE *f = fopen("test_ls.sh", "w");
	if (f) {
		fprintf(f, "#!/bin/sh\n");
		for (int i = 0; i < 1000; ++i) {
			fprintf(f, "ls >/dev/null\n");
		}
		fclose(f);
	}
	// 1. Shell script timing
	double total_elapsed_shell = 0.0;
	for (int i = 0; i < 10; ++i) {
		struct timespec ts1, ts2;
		clock_gettime(CLOCK_MONOTONIC, &ts1);
		int status = cth_exec_command((char *[]){ "sh", "./test_ls.sh", NULL });
		clock_gettime(CLOCK_MONOTONIC, &ts2);
		double elapsed = (ts2.tv_sec - ts1.tv_sec) + (ts2.tv_nsec - ts1.tv_nsec) / 1e9;
		total_elapsed_shell += elapsed;
	}
	// 2. cth_exec timing
	double total_elapsed_cth = 0.0;
	for (int i = 0; i < 10; ++i) {
		struct timespec ts1, ts2;
		clock_gettime(CLOCK_MONOTONIC, &ts1);
		for (int j = 0; j < 100; ++j) {
			cth_exec_command((char *[]){ "ls", NULL });
		}
		clock_gettime(CLOCK_MONOTONIC, &ts2);
		double elapsed = (ts2.tv_sec - ts1.tv_sec) + (ts2.tv_nsec - ts1.tv_nsec) / 1e9;
		total_elapsed_cth += elapsed;
	}

	printf("Average elapsed time for 1000x 'ls' in shell script: %.6f seconds\n", total_elapsed_shell / 10.0);
	printf("Average elapsed time for 1000x 'ls' via cth_exec(): %.6f seconds\n", total_elapsed_cth / 10.0);
	if (total_elapsed_shell > 0) {
		double percent_diff = ((total_elapsed_cth - total_elapsed_shell) / total_elapsed_shell) * 100.0;
		printf("Used %.2f%% more time than shell script\n", percent_diff);
	}
	remove("test_ls.sh");
}
int main()
{
	// Test 1
	printf("\nTest 1: ls -l\n");
	printf("  Command: ls -l\n");
	printf("  Expect: exit 0, stdout not captured\n");
	printf("Process exited with code %d\n", cth_exec_command((char *[]){ "ls", "-l", NULL }));

	// Test 2
	printf("\nTest 2: hbqvkcfdkbfukhje (invalid command)\n");
	printf("  Command: hbqvkcfdkbfukhje\n");
	printf("  Expect: exit != 0, command not found error\n");
	printf("Process exited with code %d\n", cth_exec_command((char *[]){ "hbqvkcfdkbfukhje", NULL }));

	// Test 3
	printf("\nTest 3: sh -c 'exit 19'\n");
	printf("  Command: sh -c 'exit 19'\n");
	printf("  Expect: exit 19\n");
	printf("Process exited with code %d\n", cth_exec_command((char *[]){ "sh", "-c", "exit 19", NULL }));

	// Test 4
	printf("\nTest 4: sh -c 'cat;echo hello; echo error >&2; exit 42'\n");
	printf("  Command: sh -c 'cat;echo hello; echo error >&2; exit 42'\n");
	printf("  Input: catsh stdin \n");
	printf("  get_output: true\n");
	printf("  Expect: exit 42, stdout='catsh stdin \nhello\n', stderr='error\n'\n");
	struct cth_result *res = cth_exec((char *[]){ "sh", "-c", "cat;echo hello; echo error >&2; exit 42", NULL }, "catsh stdin ", true, true);
	if (res != NULL) {
		printf("  Actual: exit code = %d\n", res->exit_code);
		if (res->stdout_ret)
			printf("  stdout: %s", res->stdout_ret);
		else
			printf("  stdout: (null)\n");
		if (res->stderr_ret)
			printf("  stderr: %s", res->stderr_ret);
		else
			printf("  stderr: (null)\n");
		cth_free_result(&res);
	} else {
		printf("  Actual: cth_exec failed\n");
	}
	int i;
	struct {
		char *desc;
		char **argv;
		char *input;
		bool get_output;
		char *expect;
	} tests[] = {
		{ "no input, no output", (char *[]){ "echo", "abc", NULL }, NULL, false, "exit 0, stdout not captured" },
		{ "no input, get output", (char *[]){ "echo", "abc", NULL }, NULL, true, "exit 0, stdout='abc\\n', stderr=''" },
		{ "with input, no output", (char *[]){ "cat", NULL }, "test input\n", false, "exit 0, stdout not captured" },
		{ "with input, get output", (char *[]){ "cat", NULL }, "test input\n", true, "exit 0, stdout='test input\\n', stderr=''" },
		{ "stderr only", (char *[]){ "sh", "-c", "echo err >&2", NULL }, NULL, true, "exit 0, stdout='', stderr='err\\n'" },
		{ "stdout and stderr", (char *[]){ "sh", "-c", "echo out; echo err >&2", NULL }, NULL, true, "exit 0, stdout='out\\n', stderr='err\\n'" },
	};
	for (i = 0; i < sizeof(tests) / sizeof(tests[0]); ++i) {
		printf("\nTest %d: %s\n", i + 5, tests[i].desc);
		printf("  Command: ");
		for (char **p = tests[i].argv; *p; ++p)
			printf("%s ", *p);
		printf("\n  Input: %s\n", tests[i].input ? tests[i].input : "(none)");
		printf("  get_output: %s\n", tests[i].get_output ? "true" : "false");
		printf("  Expect: %s\n", tests[i].expect);
		struct cth_result *res = cth_exec(tests[i].argv, tests[i].input, true, tests[i].get_output);
		if (res) {
			printf("  Actual: exit code = %d\n", res->exit_code);
			if (res->stdout_ret)
				printf("  stdout: %s", res->stdout_ret);
			else if (tests[i].get_output)
				printf("  stdout: (null)\n");
			if (res->stderr_ret)
				printf("  stderr: %s", res->stderr_ret);
			else if (tests[i].get_output)
				printf("  stderr: (null)\n");
			cth_free_result(&res);
		} else {
			printf("  Actual: cth_exec failed\n");
		}
	}
	perf_test_1();
	perf_test_2();
	perf_test_3();
	perf_test_4();
	return 0;
}