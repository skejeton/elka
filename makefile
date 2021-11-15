all:
	clang -lc *.c -o sumka -Wextra -Wall -Werror -pedantic -g -fsanitize=address
	./sumka
smol:
	clang -lc *.c -o sumka -Wextra -Wall -Werror -pedantic -O2
	strip sumka
	upx -9 sumka
