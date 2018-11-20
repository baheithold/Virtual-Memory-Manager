OOPTS = -Wall -Wextra -std=c99 -g -c
LOPTS = -Wall -Wextra -std=c99 -g

all:	fifo

fifo: 	fifo.c
	@echo Making fifo...
	@gcc $(LOPTS) fifo.c -o fifo -Wall

test:	all
	clear
	@echo Testing fifo...
	@./fifo ./addresses.txt

rebuild: 	all
	@clear
	@echo Rebuilding Project...
	@make clean -s
	@make -s
	@echo Finished Rebuilding...

valgrind:	all
	clear
	valgrind ./fifo ./addresses.txt

clean:
	@echo Cleaning...
	@rm -f *.o vgcore.* ./fifo
