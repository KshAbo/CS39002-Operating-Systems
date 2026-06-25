all: ludo.c board.c players.c
	gcc ludo.c -o ludo
	gcc board.c -o board
	gcc players.c -o players

compile: ludo.c board.c players.c
	gcc ludo.c -o ludo
	gcc board.c -o board
	gcc players.c -o players

debug: ludo.c board.c players.c
	gcc ludo.c -o ludo -DDEBUG
	gcc board.c -o board -DDEBUG
	gcc players.c -o players -DDEBUG

run: ludo 
	./ludo $(n)

clean: 
	rm ludo board players
