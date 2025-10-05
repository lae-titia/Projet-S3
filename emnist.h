#ifndef EMNIST_H
#define EMNIST_H

#include <stdint.h>

typedef struct {
    uint8_t* images;    //tableau de pixels
    uint8_t* labels;    //tableau des labels
    int num_images;
    int image_size;     
} EMNISTDataset;

//pour charger les images EMNIST from fichier IDX
int charger_emnist(const char* image_file, const char* label_file, EMNISTDataset* dataset);

//pour libérer la mémoire utilisée
void liberer_emnist(EMNISTDataset* dataset);

#endif


