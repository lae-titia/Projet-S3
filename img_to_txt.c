#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <dirent.h>
#include <string.h>
#include <glib.h>

// Définitions STB pour la conversion PNG/BMP (doivent être implémentées dans un .c du projet)
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define RATE 0.1
#define IMG_W 28
#define IMG_H 28
#define IMG_SIZE (IMG_W*IMG_H)
#define N_CLASSES 26

// Structures du réseau de neurones
struct Layer {
    int nb_neurone;
    int nb_entrees;
    double **weights;
    double *biais;
    double *values;
};

struct LayerNetwork {
    int nb_layer;
    struct Layer *network;
};

// Fonctions utilitaires
double sigmoid(double x) { return 1.0 / (1.0 + exp(-x)); }
double dsigmoid(double y) { return y * (1.0 - y); }

void softmax(double* values, int n) {
    double max = values[0];
    for (int i = 1; i < n; i++) if (values[i] > max) max = values[i];
    double sum = 0;
    for (int i = 0; i < n; i++) { values[i] = exp(values[i] - max); sum += values[i]; }
    for (int i = 0; i < n; i++) values[i] /= sum;
}

int argmax(double *arr, int n) {
    int best = 0;
    double mv = arr[0];
    for (int i = 1; i < n; i++) if (arr[i] > mv) { mv = arr[i]; best = i; }
    return best;
}

// I/O image BMP 24-bit non compressé
double* load_bmp_28x28(const char *filename)
{
    FILE *f = fopen(filename, "rb");
    if (!f) { return NULL; }

    unsigned char header[54];
    if (fread(header, sizeof(unsigned char), 54, f) != 54) { fclose(f); return NULL; }

    int width  = *(int*)&header[18];
    int height = *(int*)&header[22];

    if (width != IMG_W || height != IMG_H) {
        fprintf(stderr, "Erreur: %s doit faire %dx%d (actuellement %dx%d)\n", filename, IMG_W, IMG_H, width, height);
        fclose(f);
        return NULL;
    }

    int row_bytes = (3 * width + 3) & ~3;
    unsigned char *row = malloc(row_bytes);
    double *pixels = malloc(IMG_SIZE * sizeof(double));
    if (!row || !pixels) { fclose(f); free(row); free(pixels); return NULL; }

    for (int y = height - 1; y >= 0; y--) {
        if (fread(row, 1, row_bytes, f) != (size_t)row_bytes) { free(row); free(pixels); fclose(f); return NULL; }
        for (int x = 0; x < width; x++) {
            unsigned char b = row[3*x + 0];
            unsigned char g = row[3*x + 1];
            unsigned char r = row[3*x + 2];
            double gray = (0.299*r + 0.587*g + 0.114*b) / 255.0;
            pixels[y*width + x] = gray;
        }
    }

    free(row);
    fclose(f);
    return pixels;
}

// Conversion PNG en BMP (Nécessite STB)
int png_to_bmp(const char *png_path, const char *bmp_path)
{
    int width, height, channels;
    unsigned char *data = stbi_load(png_path, &width, &height, &channels, 3);
    if (data == NULL) {
        fprintf(stderr, "Erreur: Impossible de charger l'image PNG %s\n", png_path);
        return -1;
    }

    if (width != IMG_W || height != IMG_H) {
        fprintf(stderr, "Erreur: L'image %s n'est pas 28x28 (%dx%d)\n", png_path, width, height);
        stbi_image_free(data);
        return -1;
    }

    int success = stbi_write_bmp(bmp_path, width, height, 3, data);

    stbi_image_free(data);
    
    if (success == 0) {
        fprintf(stderr, "Erreur: Impossible de sauvegarder en BMP %s\n", bmp_path);
        return -1;
    }

    return 0;
}


// Réseau de neurones: forward
void forward(struct LayerNetwork *net, double *inputs)
{
    for (int l = 0; l < net->nb_layer; l++) {
        struct Layer *layer = &net->network[l];
        double *entrees = (l == 0) ? inputs : net->network[l - 1].values;
        for (int i = 0; i < layer->nb_neurone; i++) {
            double somme = 0.0;
            for (int j = 0; j < layer->nb_entrees; j++) somme += entrees[j] * layer->weights[i][j];
            somme += layer->biais[i];
            layer->values[i] = sigmoid(somme);
        }
    }
}

// Chargement du réseau (minimal)
struct Layer* load_network(const char *filename, int *n_layers_out) {
    FILE *f = fopen(filename, "rb");
    if (!f) { return NULL; }
    int n_layers;
    if (fread(&n_layers, sizeof(int), 1, f) != 1) { fclose(f); return NULL; }
    *n_layers_out = n_layers;
    struct Layer *layers = malloc(sizeof(struct Layer) * n_layers);
    for (int i = 0; i < n_layers; i++) {
        fread(&layers[i].nb_entrees, sizeof(int), 1, f);
        fread(&layers[i].nb_neurone, sizeof(int), 1, f);
        layers[i].weights = malloc(layers[i].nb_neurone * sizeof(double*));
        for (int ni = 0; ni < layers[i].nb_neurone; ni++) {
            layers[i].weights[ni] = malloc(layers[i].nb_entrees * sizeof(double));
            fread(layers[i].weights[ni], sizeof(double), layers[i].nb_entrees, f);
        }
        layers[i].biais = malloc(layers[i].nb_neurone * sizeof(double));
        fread(layers[i].biais, sizeof(double), layers[i].nb_neurone, f);
        layers[i].values = malloc(layers[i].nb_neurone * sizeof(double));
    }
    fclose(f);
    return layers;
}

// Fonction de prédiction
void prediction(char *test_file, char *lettre)
{
    int n_layers;
    struct Layer *layers = load_network("alphabet.nn", &n_layers);

    if (layers == NULL) {
        fprintf(stderr, "Erreur: Impossible de charger le réseau 'alphabet.nn'\n");
        lettre[0] = '?';
        return;
    }

    double *pixels = load_bmp_28x28(test_file);
    if (pixels == NULL) {
        lettre[0] = '?';
        // Libération partielle du réseau (à améliorer)
        for (int l = 0; l < n_layers; l++) {
            for (int i = 0; i < layers[l].nb_neurone; i++) free(layers[l].weights[i]);
            free(layers[l].weights);
            free(layers[l].biais);
            free(layers[l].values);
        }
        free(layers);
        return;
    }
    
    struct LayerNetwork net_temp={0};
    net_temp.nb_layer = n_layers;
    net_temp.network = layers;

    forward(&net_temp, pixels);
    softmax(net_temp.network[n_layers-1].values, N_CLASSES);
    int pred = argmax(net_temp.network[n_layers-1].values, N_CLASSES);

    free(pixels);

    for (int l = 0; l < n_layers; l++) {
        for (int i = 0; i < layers[l].nb_neurone; i++) free(layers[l].weights[i]);
        free(layers[l].weights);
        free(layers[l].biais);
        free(layers[l].values);
    }
    free(layers);
    char result = 'A' + pred;
    lettre[0] = result;
    lettre[1] = '\0';
}

// Fonction de comparaison pour g_list_sort
static gint compare_filenames(gconstpointer a, gconstpointer b)
{
    const char *str_a = (const char *)a;
    const char *str_b = (const char *)b;
    int row_a, col_a, row_b, col_b;

    if (sscanf(str_a, "row%dcol%d.png", &row_a, &col_a) != 2) return 0;
    if (sscanf(str_b, "row%dcol%d.png", &row_b, &col_b) != 2) return 0;

    if (row_a != row_b) {
        return row_a - row_b;
    } else {
        return col_a - col_b;
    }
}

// Fonction principale pour résoudre la grille
int resolve_grid(const char *folder_path, const char *output_filename, int *n_rows_out, int *n_cols_out)
{
    DIR *dir;
    struct dirent *entry;
    GList *filenames = NULL;
    int max_row = 0;
    int max_col = 0;
    int ret = 0;
    
    dir = opendir(folder_path);
    if (dir == NULL) {
        perror("Erreur lors de l'ouverture du dossier");
        return -1;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG && strstr(entry->d_name, ".png") != NULL) {
            int row, col;
            if (sscanf(entry->d_name, "row%dcol%d.png", &row, &col) == 2) {
                filenames = g_list_append(filenames, strdup(entry->d_name));
                if (row > max_row) max_row = row;
                if (col > max_col) max_col = col;
            }
        }
    }
    closedir(dir);

    if (filenames == NULL) {
        *n_rows_out = 0;
        *n_cols_out = 0;
        return -1;
    }
    
    filenames = g_list_sort(filenames, (GCompareFunc)compare_filenames);

    FILE *output_file = fopen(output_filename, "w");
    if (output_file == NULL) {
        perror("Erreur lors de l'ouverture du fichier de sortie");
        g_list_free_full(filenames, free);
        return -1;
    }

    int current_row = 0;
    char bmp_path[512];
    char predicted_char[2] = {0};

    for (GList *l = filenames; l != NULL; l = l->next) {
        char *filename = (char *)l->data;
        int row, col;
        sscanf(filename, "row%dcol%d.png", &row, &col);

        if (row != current_row) {
            if (current_row != 0) {
                fputc('\n', output_file);
            }
            current_row = row;
        }

        char png_path[512];
        snprintf(png_path, sizeof(png_path), "%s/%s", folder_path, filename);
        snprintf(bmp_path, sizeof(bmp_path), "/tmp/temp_grid_%s.bmp", filename);

        if (png_to_bmp(png_path, bmp_path) != 0) {
            ret = -1;
            continue; 
        }

        prediction(bmp_path, predicted_char); 
        
        fputc(predicted_char[0], output_file);
        
        remove(bmp_path); 
    }

    fclose(output_file);
    g_list_free_full(filenames, free);

    *n_rows_out = max_row;
    *n_cols_out = max_col;

    return ret;
}

// Fonction main (ajustée pour l'utilisation de resolve_grid)
int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <dossier_images> <fichier_sortie_txt>\n", argv[0]);
        return 1;
    }

    const char *folder_path = argv[1];
    const char *output_filename = argv[2];
    int rows = 0;
    int cols = 0;

    int result = resolve_grid(folder_path, output_filename, &rows, &cols);

    if (result == 0) {
        printf("Succès. Grille résolue dans %s (%d lignes x %d colonnes)\n", output_filename, rows, cols);
    } else {
        fprintf(stderr, "Échec de la résolution de la grille.\n");
        return 1;
    }

    return 0;
}
