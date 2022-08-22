compile:
	bison --yac --defines --output=parser.c parser.y
	flex --outfile=scanner.c scanner.l
	gcc -g -o res scanner.c parser.c symtable.c quad.c
clean:
	rm -rf parser.c scanner.c parser.h res