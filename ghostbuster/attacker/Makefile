CFLAGS := -Wall -O3 -fPIC

all:  monitor

monitor: monitor.o attacker.o train.o
	$(CC) -static -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $^

%.o: %.s
	nasm -f elf64 -o $@ $^

clean:
	rm -f attacker monitor *.o *.so
