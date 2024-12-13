CC = gcc

# Flags de compilação
CFLAGS = -g -std=c17 -D_POSIX_C_SOURCE=200809L \
         -Wall -Werror -Wextra \
         -Wcast-align -Wconversion -Wfloat-equal -Wformat=2 -Wnull-dereference -Wshadow -Wsign-conversion -Wswitch-enum -Wundef -Wunreachable-code -Wunused 
		 -fsanitize=thread
ifneq ($(shell uname -s),Darwin) # if not MacOS
    CFLAGS += -fmax-errors=5
endif

all: kvs

# Regra para compilar o arquivo principal (main.c) e vincular com os outros objetos
kvs: main.c constants.h operations.o parser.o kvs.o queue.o
	$(CC) $(CFLAGS) -o kvs main.c operations.o parser.o kvs.o queue.o  

# Regra para compilar os arquivos .c em arquivos .o
%.o: %.c %.h
	$(CC) $(CFLAGS) -c $<

# Regra para compilar o arquivo queue.c
queue.o: queue.c queue.h
	$(CC) $(CFLAGS) -c queue.c

# Comando para rodar o programa
run: kvs
	@./kvs

# Comando para limpar os arquivos objeto e executável
clean:
	rm -f *.o kvs

# Formatação do código com clang-format
format:
	@which clang-format >/dev/null 2>&1 || echo "Please install clang-format to run this command"
	clang-format -i *.c *.h
