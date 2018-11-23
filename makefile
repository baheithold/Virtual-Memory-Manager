LOPTS = -Wall -Wextra -std=c99 -g

all:	fifo lru

fifo: 	fifo.c
	@echo Making fifo...
	@gcc $(LOPTS) fifo.c -o fifo

lru: 	lru.c
	@echo Making lru...
	@gcc $(LOPTS) lru.c -o lru

test: 	all
	@echo Testing ***Should see no results from diff***
	@echo Testing fifo...
	@./fifo ./addresses.txt > fifo.out
	@diff fifo.out correct-fifo.txt
	@echo Testing lru...
	@./lru ./addresses.txt > lru.out
	@diff lru.out correct-lru.txt
	@echo Finished Testing...


test-fifo:	fifo
	@echo Testing fifo...
	@./fifo ./addresses.txt

test-lru:	lru
	@echo Testing lru...
	@./lru ./addresses.txt

rebuild: 	all
	@clear
	@echo Rebuilding Project...
	@make clean -s
	@make -s
	@echo Finished Rebuilding...

valgrind-fifo:	fifo
	clear
	valgrind ./fifo ./addresses.txt

valgrind-lru:	lru
	clear
	valgrind ./lru ./addresses.txt

clean:
	@echo Cleaning...
	@rm -f *.o vgcore.* ./fifo ./lru *.out
