#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <err.h>
#include <stdlib.h>
#include <stdio.h>
#include "traitement_image.h"
#define DEFAULT_WIDTH 800
#define DEFAULT_HEIGHT 600


#define RGB_TO_GRAY(r, g, b) (Uint8)((r * 299 + g * 587 + b * 114) / 1000)



void initialize(SDL_Window **window, SDL_Renderer **renderer, SDL_Texture **texture, char* file) 
{
	if(SDL_Init(SDL_INIT_VIDEO) != 0)
		errx(EXIT_FAILURE, "Erreur: %s", SDL_GetError());
    
    	if (IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG) == 0)
        	errx(EXIT_FAILURE, "Erreur d'initialisation de SDL_image: %s", IMG_GetError());

    	int w = DEFAULT_WIDTH;
    	int h = DEFAULT_HEIGHT;

    	SDL_Surface *loaded_surface = NULL;

	if(file != NULL)
	{
		loaded_surface = IMG_Load(file);
        	if (loaded_surface == NULL)
            		errx(EXIT_FAILURE, "Impossible de charger l'image '%s': %s", file, IMG_GetError());

       		w = loaded_surface->w;
       	 	h = loaded_surface->h;
	}
    
  	*window = SDL_CreateWindow("OurWindow", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h, 0);
    	*renderer = SDL_CreateRenderer(*window, 0, SDL_RENDERER_TARGETTEXTURE);
    
    	if(*renderer == NULL)
		errx(EXIT_FAILURE, "Erreur lors de la création du renderer: %s", SDL_GetError());
    	*texture = SDL_CreateTexture(*renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, w, h);
    	if (!*texture)
    
	    errx(EXIT_FAILURE, "Erreur lors de la création de la texture cible: %s", SDL_GetError());

	if(loaded_surface != NULL)
	{
        	SDL_Texture *texture_in_grey = surface_to_grayscale_texture(loaded_surface, *renderer);
            

		SDL_SetRenderTarget(*renderer, *texture);
		SDL_RenderCopy(*renderer, texture_in_grey, NULL, NULL);
		SDL_SetRenderTarget(*renderer, NULL);

		SDL_FreeSurface(loaded_surface);
		SDL_DestroyTexture(texture_in_grey);
	}
	else
	{
		// Si aucun fichier n'est fourni, initialise la texture en noir
		SDL_SetRenderTarget(*renderer, *texture);
       	 	SDL_SetRenderDrawColor(*renderer, 0, 0, 0, 255);
        	SDL_RenderClear(*renderer);
        	SDL_SetRenderTarget(*renderer, NULL);
	}
	


  
}


SDL_Texture* surface_to_grayscale_texture(SDL_Surface *surface, SDL_Renderer *renderer)
{

       	if (SDL_MUSTLOCK(surface)) 
       	{
		SDL_LockSurface(surface);
       	}

       	Uint32 *pixels = surface->pixels;
       	int w = surface->w;
       	int h = surface->h;
       	int pitch = surface->pitch /4;
	//int sizePixel = surface->format->BytesPerPixel;
	//int total_pitch = 0;
	//int total_h = 0;
    	// Convertion en gris

    	for (int i = 0; i < h; i++)
    	{
		for (int j = 0; j < w;j++)
		{
       	 		Uint8 r, g, b, a;
       			SDL_GetRGBA(pixels[i * pitch + j], surface->format, &r, &g, &b, &a);
       			Uint8 gray = RGB_TO_GRAY(r, g, b);
			pixels[i * pitch+ j] = SDL_MapRGBA(surface->format, gray, gray, gray, a);
    		}
		//total_h += w;
		//total_pitch += pitch;
	}

   	 if (SDL_MUSTLOCK(surface)) {
        	SDL_UnlockSurface(surface);
    	}
    
        	SDL_Texture *new_Texture = SDL_CreateTextureFromSurface(renderer, surface);
    	if (!new_Texture)
    	    errx(EXIT_FAILURE, "Erreur lors de la création de la texture: %s", SDL_GetError());
    
   	 return new_Texture;
}

void save_texture_to_file(SDL_Renderer *renderer, SDL_Texture *texture, const char *filename) 
{
    	int w, h;
    	SDL_QueryTexture(texture, NULL, NULL, &w, &h);
	
	SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_RGBA8888);
	if(!surface)
		errx(EXIT_FAILURE, "Erreur lors de la création de la surface: %s", SDL_GetError());
   	
	
	SDL_SetRenderTarget(renderer, texture);

	if(SDL_RenderReadPixels(renderer, NULL, SDL_PIXELFORMAT_RGBA8888, surface->pixels, surface->pitch) != 0)
	{
		SDL_FreeSurface(surface);
		errx(EXIT_FAILURE, "Erreur lors de la lecture des pixels: %s", SDL_GetError());
	}	
	SDL_SetRenderTarget(renderer, NULL);	
    	printf("Sauvegarde de l'image finale en niveaux de gris sous : %s\n", filename);
	if(SDL_SaveBMP(surface, filename) != 0)
	{
       	 	SDL_FreeSurface(surface);
       		errx(EXIT_FAILURE, "Erreur lors de la sauvegarde du BMP: %s", SDL_GetError());
    	}
	SDL_FreeSurface(surface);
}




void terminate(SDL_Window *window, SDL_Renderer *renderer, SDL_Texture *texture) 
{

	SDL_DestroyTexture(texture);

	SDL_DestroyRenderer(renderer);

	SDL_DestroyWindow(window);

    	IMG_Quit();

	SDL_Quit();

}


char* make_grayscale_image(char* argv) 
{
 	SDL_Window *window = NULL;
 	SDL_Renderer *renderer = NULL;
 	SDL_Texture *texture = NULL;
  	char* file = argv;
  	char* output_name = "grayscale_image";
  	initialize(&window, &renderer, &texture, file);
  	save_texture_to_file(renderer, texture, output_name);
 	terminate(window, renderer, texture);
	return output_name;
    
}
