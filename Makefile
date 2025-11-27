CC = gcc

# Flags de compilation
CFLAGS_GTK = -Wall -Wextra -g -O0 `pkg-config --cflags gtk+-3.0 glib-2.0 gdk-pixbuf-2.0`
CFLAGS_SDL = -Wall -Wextra -g -O0 `pkg-config --cflags sdl2 SDL2_image gtk+-3.0 glib-2.0 gdk-pixbuf-2.0`

# Flags d'édition de liens
LDFLAGS_GTK = `pkg-config --libs gtk+-3.0 glib-2.0 gdk-pixbuf-2.0` -lm
LDFLAGS_SDL = `pkg-config --libs sdl2 SDL2_image gtk+-3.0 glib-2.0 gdk-pixbuf-2.0` -lm

# Fichiers objets - NOM COHERENT
OBJS_INTERFACE = interface_v1.o neurone_systemImage.o traitement_image.o segmenter.o png_to_bmp.o
OBJS_SOLVER = solver.o

# Cibles principales
all: interface solver

# Interface GTK
interface: $(OBJS_INTERFACE)
	$(CC) -o $@ $^ $(LDFLAGS_GTK) $(LDFLAGS_SDL)

# RÈGLES CORRIGÉES - noms cohérents
interface_v1.o: interface_v1.c interface_v1.h neurone_systemImage.h traitement_image.h segmenter.h
	$(CC) $(CFLAGS_GTK) $(CFLAGS_SDL) -c interface_v1.c

neurone_systemImage.o: neurone_systemImage.c neurone_systemImage.h png_to_bmp.h
	$(CC) $(CFLAGS_GTK) -c neurone_systemImage.c

png_to_bmp.o: png_to_bmp.c png_to_bmp.h
	$(CC) $(CFLAGS_SDL) -c png_to_bmp.c

traitement_image.o: traitement_image.c traitement_image.h
	$(CC) $(CFLAGS_SDL) -c traitement_image.c

segmenter.o: segmenter.c segmenter.h
	$(CC) $(CFLAGS_SDL) -c segmenter.c

# Solver
solver: $(OBJS_SOLVER)
	$(CC) -o $@ $^ -lm

solver.o: solver.c solver.h
	$(CC) -Wall -Wextra -g -O0 -c solver.c

# Nettoyage
clean:
	rm -f *.o interface solver grayscale_image grayscale_rotated.bmp results.txt alphabet.nn

rebuild: clean all

.PHONY: all clean rebuild
