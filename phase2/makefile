compile:
	bison -v --yac --defines --output=parser.c parser.y
	flex --outfile=scanner.c scanner.l
	gcc -g -o res scanner.c parser.c symtable.c
	./res input.txt
clean:
	rm -rf parser.c
	rm -rf scanner.c
	rm -rf parser.h	
	rm res