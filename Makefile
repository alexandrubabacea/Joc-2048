# Programarea Calculatoarelor, seria CC
# Tema2 - 2048
# Băbacea Alexandru

build: 2048

2048: 2048.c
	gcc -Wall 2048.c -o 2048 -lcurses -lm


.PHONY:

clean:
	rm -f 2048

run:
	./2048
