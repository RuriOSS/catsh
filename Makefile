all:
	cc tests/test.c src/catsh.c -o test
	cc tests/test_cat_1.c -o test_cat_1
	cc tests/test_cat_2.c -o test_cat_2
	cc -fsanitize=address,undefined tests/demo.c src/catsh.c -o demo
format:
	clang-format -i src/include/*.h src/*.c tests/*.c
test: all
	./test
check:
	clang-tidy --checks=*,-clang-analyzer-security.insecureAPI.strcpy,-altera-unroll-loops,-cert-err33-c,-concurrency-mt-unsafe,-clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling,-readability-function-cognitive-complexity,-cppcoreguidelines-avoid-magic-numbers,-readability-magic-numbers,-bugprone-easily-swappable-parameters,-cert-err34-c,-misc-include-cleaner,-readability-identifier-length,-bugprone-signal-handler,-cert-msc54-cpp,-cert-sig30-c,-altera-id-dependent-backward-branch,-bugprone-suspicious-realloc-usage,-hicpp-signed-bitwise,-clang-analyzer-security.insecureAPI.UncheckedReturn,-bugprone-reserved-identifier,-cert-dcl37-c,-cert-dcl51-cpp,-google-readability-function-size,-hicpp-function-size,,-google-readability-todo,-readability-function-size,-bugprone-reserved-identifier,-cert-dcl37-c,-cert-dcl51-cpp src/catsh.c --