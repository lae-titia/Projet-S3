#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>


//Fonctions trouver sur internet pour les images de formats PPM. A ameliorer.

Image* NouvelleImage(int w,int h)
{
	Image* I = malloc(sizeof(Image));
	I->w = w;
	I->h = h;
	I->dat = calloc(1,w*h*sizeof(Pixel*));
	return I;
}


Pixel GetPixel(Image* I,int i,int j)
{
	assert(I && i>=0 && i<I->w && j>=0 && j<I->h);
	return I->dat[I->w*j+i];
}

Image* Charger(const char* fichier)
{
	int i,j,max;
	char buf[10];
	Image* I;
	FILE* F = fopen(fichier,"r");
	if (!F)
		return NULL;
	fscanf(F,"%s %d %d %d",buf,&i,&j,&max);
	I = NouvelleImage(i,j);
	for(i=0;i<I->w*I->h;i++)
	{
		int r,g,b;
		fscanf(F,"%d %d %d",&r,&g,&b);
		I->dat[i].r = (unsigned char)r;
		I->dat[i].g = (unsigned char)g;
		I->dat[i].b = (unsigned char)b;
	}
	fclose(F);
	return I;
}

Image* SuppressionCouleur(Image* img)
{
	Image* I;
	I = NouvelleImage(img->w,img->h);
	for (int i = 0; i < I->w * I->h; i++) {
    	unsigned char r = I->dat[i].r;
    	unsigned char g = I->dat[i].g;
    	unsigned char b = I->dat[i].b;

	//Passer l'image en niveau de gris
 
    	unsigned char gray = (unsigned char)(0.299 * r + 0.587 * g + 0.114 * b);
	
    	I->dat[i].r = gray;
    	I->dat[i].g = gray;
    	I->dat[i].b = gray;
	}
	//Passer l'image en noir et blanc 

	for (int i = 0; i < I->w * I->h; i++) {
    	unsigned char gray = I->dat[i].r; 

    	if (gray > 128) {
        	I->dat[i].r = 255;
        	I->dat[i].g = 255;
        	I->dat[i].b = 255;
    	}
	 else {
        	I->dat[i].r = 0;
        	I->dat[i].g = 0;
        	I->dat[i].b = 0;
    }
}
	return I;

}


int Sauver(Image* I,const char* fichier)
{
	int i;
	FILE* F = fopen(fichier,"w");
	if (!F)
		return -1;
	fprintf(F,"P3\n%d %d\n255\n",I->w,I->h);
	for(i=0;i<I->w*I->h;i++)
		fprintf(F,"%d %d %d ",I->dat[i].r,I->dat[i].g,I->dat[i].b);
	fclose(F);
	return 0;
}


// Ajouter les fonctions du TP 1 pour les fichiers de format BMP.



