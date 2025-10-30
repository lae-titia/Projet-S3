#ifndef TRAITEMENT_H
#define TRIATEMENT_H
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h> 
SDL_Texture* surface_to_grayscale_texture(SDL_Surface *surface, SDL_Renderer *renderer);
void save_texture_to_file(SDL_Renderer *renderer, SDL_Texture *texture, const char *filename);
void terminate(SDL_Window *window, SDL_Renderer *renderer, SDL_Texture *texture);
char* make_grayscale_image(char* argv);

#endif
