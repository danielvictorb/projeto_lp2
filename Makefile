CC = gcc
CFLAGS = -Wall -Wextra -pthread
SRC = src/main.c src/sincronizacao.c
OUT = restaurante

all: $(OUT)

$(OUT): $(SRC)
	$(CC) $(CFLAGS) -o $(OUT) $(SRC)

run: $(OUT)
	./$(OUT)

clean:
	rm -f $(OUT)
	rm -f resultados/log_execucao.txt

.PHONY: all run clean
