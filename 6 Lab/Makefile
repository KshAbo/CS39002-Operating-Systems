all: manager.c demon.c nomad.c
	gcc manager.c -o manager
	gcc demon.c -o demon
	gcc nomad.c -o nomad

demorun: manager demon nomad
	./manager &
	./demon 5 &
	./nomad 4

clean:
	rm manager demon nomad
