src := $(wildcard src/*.c)
out := main
cflags := -lraylib -lm 

all:  build run
build: $(src)
	gcc $(src) -o $(out) $(cflags)
run: 
	./$(out)
clean:
	rm $(out)