CC = gcc

# Flags de compilation
CFLAGS_GTK = -Wall -Wextra `pkg-config --cflags gtk+-3.0 glib-2.0 gdk-pixbuf-2.0`
CFLAGS_SDL = -Wall -Wextra `pkg-config --cflags sdl2 SDL2_image gtk+-3.0 glib-2.0 gdk-pixbuf-2.0`

# Flags d’édition de liens
LDFLAGS_GTK = `pkg-config --libs gtk+-3.0 glib-2.0 gdk-pixbuf-2.0` -lm
LDFLAGS_SDL = `pkg-config --libs sdl2 SDL2_image gtk+-3.0 glib-2.0 gdk-pixbuf-2.0` -lm

# Fichiers objets
OBJS_INTERFACE = interface_v1.o neurone_system.o traitement_image.o segmenter.o
OBJS_SOLVER = solver.o

# Cibles principales
all: interface solver

# Interface GTK
interface: $(OBJS_INTERFACE)
	$(CC) -o $@ $^ $(LDFLAGS_GTK) $(LDFLAGS_SDL)

interface_v1.o: interface_v1.c interface_v1.h neurone_system.h traitement_image.h segmenter.h
	$(CC) $(CFLAGS_GTK) $(CFLAGS_SDL) -c interface_v1.c

neurone_system.o: neurone_system.c neurone_system.h
	$(CC) $(CFLAGS_GTK) -c neurone_system.c

traitement_image.o: traitement_image.c traitement_image.h
	$(CC) $(CFLAGS_SDL) -c traitement_image.c

segmenter.o: segmenter.c segmenter.h
	$(CC) $(CFLAGS_SDL) -c segmenter.c

# Solver
solver: $(OBJS_SOLVER)
	$(CC) -o $@ $^ -lm

solver.o: solver.c solver.h
	$(CC) -Wall -Wextra -c solver.c

# Nettoyage
clean:
	rm -f *.o interface solver grayscale_image grayscale_rotated.bmp results.txt

rebuild: clean all

.PHONY: all clean rebuild
