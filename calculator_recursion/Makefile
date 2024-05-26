CC = gcc
CXX = g++
CFLAGS = -O3
exe = main
obj = main.o codeGen.o lex.o parser.o

$(exe): $(obj)
	$(CC) -o $(exe) $(obj)
  
%.o: %.c
	$(CC) -c $^ -o $@ $(CFLAGS)

clean:
	rm $(exe) $(obj)