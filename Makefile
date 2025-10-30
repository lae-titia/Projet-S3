# Compilateur
CC = gcc

# Options de compilation
CFLAGS = -Wall -Wextra -Werror

# --- Programme 1 : solver ---
SOLVER_TARGET = solver
SOLVER_SRCS = main.c solver.c

#--- Programme 2 : interface ---
INTERFACE_TARGET = interface
INTERFACE_SRCS = neurone_system.c interface_v1.c
GTK_CFLAGS = $(shell pkg-config --cflags gtk+-3.0)
GTK_LIBS = $(shell pkg-config --libs gtk+-3.0)

# --- Règle principale ---
all: $(SOLVER_TARGET) $(INTERFACE_TARGET)

# Compilation du solver
$(SOLVER_TARGET): $(SOLVER_SRCS)
	$(CC) $(CFLAGS) $(SOLVER_SRCS) -o $(SOLVER_TARGET) -lm

# Compilation de l’interface GTK
$(INTERFACE_TARGET): $(INTERFACE_SRCS)
	$(CC) $(CFLAGS) $(GTK_CFLAGS) $(INTERFACE_SRCS) -o $(INTERFACE_TARGET) $(GTK_LIBS) -lm

FINAL:
	gcc -o FINAL interface_v1.c neurone_system.c traitement_image.c segmenter.c $$(pkg-config --cflags --libs gtk+-3.0) $$(pkg-config glib-2.0 --libs) -lSDL2_image -lSDL2 -lm
# Nettoyage
clean:
	rm -f $(SOLVER_TARGET) $(INTERFACE_TARGET) *.o

.PHONY: all clean
