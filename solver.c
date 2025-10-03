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

void solver(char* file,char* word)
{

	int cursor = 0;
	while (word[cursor] != '\0')
	{
		if(word[cursor] >= 97)
		       word[cursor] -= 32;
		cursor++;	
	}

	printf("Reading grid...\n");
	FILE* fileToRead = fopen(file,"r");
	if (fileToRead == NULL)
		errx(1,"file %s do not exist or can't be read", file);
	char* line;
	size_t nbColone = 0;
	size_t lineSize = 0;
	size_t nbLigne = 0;
	while(getline(&line,&lineSize,fileToRead) != -1)
	{
		nbLigne++;
	}
	while(line[nbColone] != '\0')
	{
		nbColone++;
	}
	nbColone--;

	

	char grid[nbLigne][nbColone];
	rewind(fileToRead);
	int i = 0;
	while(getline(&line,&lineSize,fileToRead) != -1)
	{

		strcpy(grid[i],line);
		i++;
	}
	printf("Searching word...\n");

	for (size_t k = 0; k < nbLigne;k++)
	{
		for(size_t j = 0; j < nbColone ; j++)
		{

			if(word[0] == grid[k][j])
			{
				size_t o = 0;
				int forward = 1;
				int backward = 1;
				int up = 1;
				int down = 1;
				int DLup = 1;
				int DRup = 1;
				int DLdown = 1;
				int DRdown = 1;
				while(word[o] != '\0')
				{
					if (forward && j+o < nbColone)
					{
						if (word[o] != grid[k][j+o])
						{
							forward = 0;
						}
					}
					else
					{
						forward = 0;
					}
					//
					if (backward && j >= o)
					{
						if (word[o] != grid[k][j-o])
						{
							backward = 0;
						}
					}
					else
					{
						backward = 0;
					}
					//
					if (down && k+o < nbLigne)
					{
						if (word[o] != grid[k+o][j])
						{
							down = 0;
						}
					}
					else
					{
						down = 0;
					}
					//
						
					if (up && k >= o)
					{
						if (word[o] != grid[k-o][j])
						{
							up = 0;
						}
					}
					else
					{
						up = 0;
					}
					//
					if (DLup && k >= o && j >= o)
					{
						if (word[o] != grid[k-o][j-o])
						{
							DLup = 0;
						}
					}
					else
					{
						DLup = 0;
					}
					//
					if (DRup && j+o < nbColone && k >= o)
					{
						if (word[o] != grid[k-o][j+o])
						{
							DRup = 0;
						}
					}
					else
					{
						DRup = 0;
					}
					//
					if (DLdown && k+o < nbLigne && j >= o)
					{
						if (word[o] != grid[k+o][j-o])
						{
							DLdown = 0;
						}
					}
					else
					{
						DLdown = 0;
					}
					//
					if (DRdown && k+o < nbLigne  && j+o < nbColone)
					{
						if (word[o] != grid[k+o][j+o])
						{
							DRdown = 0;
						}
					}
					else
					{
						DRdown = 0;
					}
					//
					o++;
				}
				o--;
				if (forward)
				{
					printf("(%lu,%lu),(%lu,%lu)\n",j,k,j+o,k);
					return;
				}
				else if (backward)
				{
					printf("(%lu,%lu),(%lu,%lu)\n",j,k,j-o,k);
					return;
				}
				else if (up)
				{
					printf("(%lu,%lu),(%lu,%lu)\n",j,k,j,k-o);
					return;
				}
				else if (down)
				{
					printf("(%lu,%lu),(%lu,%lu)\n",j,k,j,k+o);
					return;
				}
				else if (DLup)
				{
					printf("(%lu,%lu),(%lu,%lu)\n",j,k,j-o,k-o);
					return;
				}
				else if (DRup)
				{
					printf("(%lu,%lu),(%lu,%lu)\n",j,k,j+o,k-o);
					return;
				}
				else if (DLdown)
				{
					printf("(%lu,%lu),(%lu,%lu)\n",j,k,j-o,k+o);
					return;
				}
				else if (DRdown)
				{
					printf("(%lu,%lu),(%lu,%lu)\n",j,k,j+o,k+o);
					return;
				}
			
			}	
		}
	}
	printf("Not Found\n");
	return;
}

int main(int argc,char** const argv)
{
	if (argc != 3)
	{
		errx(2,"wrong number of arguments");
	}
	char* fileName = argv[1];
	char* word = argv[2];
	solver(fileName,word);
	return 0;
}




// Ajouter les fonctions du TP 1 pour les fichiers de format BMP.



