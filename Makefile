solver : solver.c
	gcc main.c solver.c -o solver -Wall -Wextra -Werror

# Nom de l'exécutable
TARGET = interface

# Fichiers source
SRCS = neurone_system.c interface_v1.c

# Options de compilation
CFLAGS = -Wall -Wextra $(shell pkg-config --cflags gtk+-3.0)

# Options de linkage
LIBS = $(shell pkg-config --libs gtk+-3.0)

# Règle principale
all: $(TARGET)

$(TARGET): $(SRCS)
	gcc $(CFLAGS) $(SRCS) -o $(TARGET) $(LIBS)

# Nettoyage
clean:
	rm -f $(TARGET) *.o

.PHONY: all clean
