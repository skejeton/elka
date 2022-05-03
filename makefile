TARGET=elka
WARNS=-Wall

all:
	$(CC) -lc *.c -o $(TARGET) $(WARNS) -g -fsanitize=address
	./$(TARGET)
cachegrind:
	$(CC) -lc *.c -o $(TARGET) $(WARNS) -g
	valgrind --tool=cachegrind ./$(TARGET)
	kcachegrind cachegrind.out.*
	rm cachegrind.out.*
smol:
	$(CC) -lc *.c -o $(TARGET) -Wextra -Wall -Werror -pedantic -O2
	strip $(TARGET)
	upx -9 $(TARGET)
fast:
	clang -lc bundle/bundle.c -o $(TARGET) -Wextra -Wall -Werror -pedantic -O3 -flto
