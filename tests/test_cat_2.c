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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define BUF_SIZE 4096
// This program reads from stdin and writes half of the input to stdout.
// Also prefixes the output with 114 'A's.
// This is used to test when output is not the same size as input, and not aligned to 2^n.
int main(void)
{
	unsigned char seed[114];
	memset(seed, 0x41, 114);
	unsigned char buffer[BUF_SIZE];
	size_t n;
	while ((n = fread(buffer, 1, BUF_SIZE, stdin)) > 0) {
		fwrite(seed, 1, 114, stdout);
		if (fwrite(buffer, 1, n / 2, stdout) != n / 2) {
			perror("fwrite");
			return 1;
		}
	}
	if (ferror(stdin)) {
		perror("fread");
		return 1;
	}
	return 0;
}