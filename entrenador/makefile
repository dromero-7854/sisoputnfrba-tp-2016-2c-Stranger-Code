#Makefile para el modelo Cliente-Servidor presentado como ejemplo.

COMPILER=gcc
BIN_PATH=Debug

all:
	mkdir -p $(BIN_PATH)
	$(COMPILER) src/Entrenador.c -o $(BIN_PATH)/Entrenador -lcommons

clean:
	rm -rf Debug

client:
	mkdir -p $(BIN_PATH)
	$(COMPILER) src/Entrenador.c -o $(BIN_PATH)

debug: 
	COMPILER += -DDEBUG -g
debug: all

# Contemplamos algunos Alias para subsanar posibles diferencias de escritura

entrenador:
	make client

# Sistemas Operativos, Universidad Tecnologica Nacional, Facultad Regional Buenos Aires. 2014.