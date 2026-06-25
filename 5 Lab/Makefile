all: manager.c launcher.c timer.c
	gcc manager.c -o manager
	gcc launcher.c -o launcher
	gcc process.c -o process
	gcc timer.c -o timer

debug: manager.c launcher.c timer.c
	gcc manager.c -o manager -DDEBUG
	gcc launcher.c -o launcher -DDEBUG
	gcc process.c -o process -DDEBUG
	gcc timer.c -o timer -DDEBUG

run: manager timer launcher process
	./manager & 
	sleep 0.5 & 
	./timer &
	sleep 0.5 & 
	./launcher

clean:
	rm manager launcher timer process
