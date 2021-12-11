all:
	clang -lc *.c -o sumka -Wextra -Wall -Werror -pedantic -g -fsanitize=address
	./sumka
cachegrind:
	clang -lc *.c -o sumka -Wextra -Wall -Werror -pedantic -g
	valgrind --tool=cachegrind ./sumka
	kcachegrind cachegrind.out.*
	rm cachegrind.out.*
smol:
	clang -lc *.c -o sumka -Wextra -Wall -Werror -pedantic -O2
	strip sumka
	upx -9 sumka
fast:
	clang -lc bundle/bundle.c -o sumka -Wextra -Wall -Werror -pedantic -O3 -flto
