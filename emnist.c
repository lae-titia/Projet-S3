#include "emnist.h"
#include <stdio.h>
#include <stdlib.h>
//sert à charger et libérer les données emnist donc les caractères 
//avec on peut lire les images et labels, 
//Fonction pour lire un entiers 32 bits depuis un fichier 
static uint32_t read_be_uint32(FILE* f) {
    uint8_t b[4];
    fread(b, 1, 4, f);
    return (b[0] << 24) | (b[1] << 16) | (b[2] << 8) | b[3];
}

//chargement IDX EMNIST (erreurs non gérées ici pour exemple)
int charger_emnist(const char* image_file, const char* label_file, EMNISTDataset* dataset) {
    FILE* f_images = fopen(image_file, "rb");
    FILE* f_labels = fopen(label_file, "rb");
    if (!f_images || !f_labels) return -1;

    uint32_t magic_images = read_be_uint32(f_images);
    uint32_t num_images = read_be_uint32(f_images);
    uint32_t rows = read_be_uint32(f_images);
    uint32_t cols = read_be_uint32(f_images);

    uint32_t magic_labels = read_be_uint32(f_labels);
    uint32_t num_labels = read_be_uint32(f_labels);

    if (num_images != num_labels) return -1;

    dataset->num_images = num_images;
    dataset->image_size = rows * cols;
    dataset->images = malloc(num_images * dataset->image_size);
    dataset->labels = malloc(num_images);

    fread(dataset->images, 1, num_images * dataset->image_size, f_images);
    fread(dataset->labels, 1, num_images, f_labels);

    fclose(f_images);
    fclose(f_labels);


    return 0;
}

void liberer_emnist(EMNISTDataset* dataset) {
    free(dataset->images);
    free(dataset->labels);
    dataset->images = NULL;
    dataset->labels = NULL;
}
