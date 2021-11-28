all:
	clang -lc *.c -o sumka -Wextra -Wall -Werror -pedantic -g -fsanitize=address
	./sumka
cachegrind:
	clang -lc *.c -o sumka -Wextra -Wall -Werror -pedantic -g
	valgrind --tool=callgrind --dump-instr=yes ./sumka
	kcachegrind callgrind.out.*
	rm callgrind.out.*
smol:
	clang -lc *.c -o sumka -Wextra -Wall -Werror -pedantic -O2
	strip sumka
	upx -9 sumka
fast:
	clang -lc bundle/bundle.c -o sumka -Wextra -Wall -Werror -pedantic -O3 -flto
