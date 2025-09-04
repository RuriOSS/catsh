all:
	cc tests/test.c src/catsh.c -o test
	cc tests/test_cat_1.c -o test_cat_1
	cc tests/test_cat_2.c -o test_cat_2
	cc tests/demo.c src/catsh.c -o demo
format:
	clang-format -i src/include/*.h src/*.c tests/*.c
test: all
	./test