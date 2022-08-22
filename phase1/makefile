CC = gcc
s = scanner
in = input
test = al

compile:
	flex --outfile=$(s).c $(s).l
	$(CC) -o $(s) $(s).c
	./$(s) $(in).txt
clean:
	rm -rf  *.c
	rm $(s)