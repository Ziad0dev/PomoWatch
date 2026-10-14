CC ?= cc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Wpedantic
CORE = firmware/pomo.c firmware/button.c

test: tests/test_pomo
	./tests/test_pomo

tests/test_pomo: tests/test_pomo.c $(CORE)
	$(CC) $(CFLAGS) -o $@ $^

sim/pomosim: sim/sim.c firmware/pomo.c
	$(CC) $(CFLAGS) -o $@ $^

sim: sim/pomosim
	./sim/pomosim

clean:
	rm -f tests/test_pomo sim/pomosim

.PHONY: test sim clean
