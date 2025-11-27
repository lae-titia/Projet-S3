// train_letters.c
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <dirent.h>
#include <string.h>
#include <glib.h>


#include "neurone_systemImage.h"
#include "png_to_bmp.h"

#define PASSAGE 1000   // baisse pour tests rapides, remonte si tu veux
#define RATE 0.1

#define IMG_W 28
#define IMG_H 28
#define IMG_SIZE (IMG_W*IMG_H)
#define N_CLASSES 26



// -------------------- utilitaires --------------------
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

// -------------------- I/O image BMP 24-bit non compressé --------------------
double* load_bmp_28x28(const char *filename)
{
    FILE *f = fopen(filename, "rb");
    if (!f) { perror(filename); return NULL; }

    unsigned char header[54];
    if (fread(header, sizeof(unsigned char), 54, f) != 54) { fclose(f); return NULL; }

    int width  = *(int*)&header[18];
    int height = *(int*)&header[22];

    if (width != IMG_W || height != IMG_H) {
        fprintf(stderr, "Erreur: %s doit faire %dx%d (actuellement %dx%d)\n", filename, IMG_W, IMG_H, width, height);
        fclose(f);
        return NULL;
    }

    // BMP rows are padded to 4 bytes; compute padding
    int row_bytes = (3 * width + 3) & ~3;
    unsigned char *row = malloc(row_bytes);
    double *pixels = malloc(IMG_SIZE * sizeof(double));
    if (!row || !pixels) { perror("malloc"); fclose(f); free(row); free(pixels); return NULL; }

    // BMP stores pixels bottom-up
    for (int y = height - 1; y >= 0; y--) {
        if (fread(row, 1, row_bytes, f) != (size_t)row_bytes) { perror("fread row"); free(row); free(pixels); fclose(f); return NULL; }
        for (int x = 0; x < width; x++) {
            unsigned char b = row[3*x + 0];
            unsigned char g = row[3*x + 1];
            unsigned char r = row[3*x + 2];
            // conversion en gris
            double gray = (0.299*r + 0.587*g + 0.114*b) / 255.0;
            pixels[y*width + x] = gray;
        }
    }

    free(row);
    fclose(f);
    return pixels;
}

// -------------------- réseau --------------------
void init_layer(struct Layer *layer, int nb_input, int nb_neurones)
{
    layer->nb_neurone = nb_neurones;
    layer->nb_entrees = nb_input;
    layer->weights = malloc(nb_neurones * sizeof(double*));
    layer->biais = malloc(nb_neurones * sizeof(double));
    layer->values = malloc(nb_neurones * sizeof(double));
    for (int i = 0; i < nb_neurones; i++) {
        layer->biais[i] = ((double)rand() / RAND_MAX) - 0.5;
        layer->values[i] = 0.0;
        layer->weights[i] = malloc(nb_input * sizeof(double));
        for (int j = 0; j < nb_input; j++)
            layer->weights[i][j] = ((double)rand() / RAND_MAX) - 0.5;
    }
}

struct LayerNetwork *network(int nb_layers, int* nb_neurones_layers, int nb_neurones_init){
    struct LayerNetwork *N_network = malloc(sizeof(struct LayerNetwork));
    N_network->nb_layer= nb_layers;
    N_network->network = malloc(nb_layers*sizeof(struct Layer));
    for(int i = 0; i < nb_layers; i++) {
        struct Layer *layer = &N_network->network[i];
        if(i == 0) init_layer(layer, nb_neurones_init, nb_neurones_layers[i]);
        else init_layer(layer, nb_neurones_layers[i-1], nb_neurones_layers[i]);
    }
    return N_network;
}

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

// backprop for vector target (one-hot)
void backprop_multi(struct LayerNetwork *net, double *inputs, double *targets)
{
    int L = net->nb_layer;
    double **delta = malloc(L * sizeof(double*));
    for (int i = 0; i < L; i++) delta[i] = NULL;

    struct Layer *output = &net->network[L - 1];
    delta[L - 1] = malloc(output->nb_neurone * sizeof(double));
    // output layer delta
    for (int i = 0; i < output->nb_neurone; i++) {
        double err = targets[i] - output->values[i];
        delta[L - 1][i] = err * dsigmoid(output->values[i]);
    }

    // hidden layers
    for (int l = L - 2; l >= 0; l--) {
        struct Layer *layer = &net->network[l];
        struct Layer *next = &net->network[l + 1];
        delta[l] = malloc(layer->nb_neurone * sizeof(double));
        for (int i = 0; i < layer->nb_neurone; i++) {
            double somme = 0.0;
            for (int j = 0; j < next->nb_neurone; j++)
                somme += next->weights[j][i] * delta[l + 1][j];
            delta[l][i] = somme * dsigmoid(layer->values[i]);
        }
    }

    // update weights
    for (int l = 0; l < L; l++) {
        struct Layer *layer = &net->network[l];
        double *entrees = (l == 0) ? inputs : net->network[l - 1].values;
        for (int i = 0; i < layer->nb_neurone; i++) {
            for (int j = 0; j < layer->nb_entrees; j++)
                layer->weights[i][j] += RATE * delta[l][i] * entrees[j];
            layer->biais[i] += RATE * delta[l][i];
        }
    }

    for (int l = 0; l < L; l++) if (delta[l]) free(delta[l]);
    free(delta);
}

void traning(struct LayerNetwork *ln, double** inputs, double** outputs, int size_input)
{
    for (int epoch = 0; epoch < PASSAGE; epoch++) {
        for (int i = 0; i < size_input; i++) {
            forward(ln, inputs[i]);
            backprop_multi(ln, inputs[i], outputs[i]);
        }
        if ((epoch+1) % 10 == 0) printf("Epoch %d/%d done\n", epoch+1, PASSAGE);
    }
}

// -------------------- save / load (inchangés) --------------------
void save_network(struct Layer *layers, int n_layers, const char *filename) {
    FILE *f = fopen(filename, "wb");
    if (!f) { perror("fopen save"); return; }
    fwrite(&n_layers, sizeof(int), 1, f);
    for (int i = 0; i < n_layers; i++) {
        fwrite(&layers[i].nb_entrees, sizeof(int), 1, f);
        fwrite(&layers[i].nb_neurone, sizeof(int), 1, f);
        for (int ni = 0; ni < layers[i].nb_neurone; ni++) {
            fwrite(layers[i].weights[ni], sizeof(double), layers[i].nb_entrees, f);
        }
        fwrite(layers[i].biais, sizeof(double), layers[i].nb_neurone, f);
    }
    fclose(f);
}

// minimal load (kept for prediction use)
struct Layer* load_network(const char *filename, int *n_layers_out) {
    FILE *f = fopen(filename, "rb");
    if (!f) { perror("fopen load"); return NULL; }
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

void principal(void)
{

    printf("coucou");
    char letters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    convert_folder("./lettres");

    srand(1234); 

    int nb_total_images = 0;
    for (int l = 0; l < N_CLASSES; l++) {
        char folder[128];
        sprintf(folder, "./lettres/%c/", letters[l]);
        DIR* dir = opendir(folder);
        if (!dir) continue;
        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_type != DT_REG) continue;
            nb_total_images++;
        }
        closedir(dir);
    }

    if (nb_total_images == 0) {
        fprintf(stderr, "Aucune image trouvée dans les dossiers A..Z\n");
        return;
    }
    printf("Images trouvées: %d\n", nb_total_images);

    double **inputs = malloc(nb_total_images * sizeof(double*));
    double **outputs = malloc(nb_total_images * sizeof(double*));
    int index = 0;
    for (int l = 0; l < N_CLASSES; l++) {
        char folder[128];
        sprintf(folder, "./lettres/%c/", letters[l]);
        DIR* dir = opendir(folder);
        if (!dir) continue;
        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_type != DT_REG) continue;
            char path[256];
            sprintf(path, "%s%s", folder, entry->d_name);
            double *pix = load_bmp_28x28(path);
            if (!pix) continue;
            inputs[index] = pix;
            outputs[index] = calloc(N_CLASSES, sizeof(double));
            outputs[index][l] = 1.0;
            index++;
        }
        closedir(dir);
    }
    int total = index;
    printf("Chargé %d images valides\n", total);

    int nb_layers = 3;
    int neurons_per_layer[] = {128, 64, N_CLASSES};
    struct LayerNetwork *net = network(nb_layers, neurons_per_layer, IMG_SIZE);

    printf("Démarrage entraînement...\n");
    traning(net, inputs, outputs, total);
    printf("Entraînement terminé\n");

    int correct = 0;
    for (int i = 0; i < total; i++) {
        forward(net, inputs[i]);
        softmax(net->network[nb_layers-1].values, N_CLASSES);
        int pred = argmax(net->network[nb_layers-1].values, N_CLASSES);
        int true_label = -1;
        for (int j = 0; j < N_CLASSES; j++) if (outputs[i][j] == 1.0) { true_label = j; break; }
        if (pred == true_label) correct++;
    }
    double accuracy = 100.0 * (double)correct / total;
    printf("Accuracy sur l'ensemble: %.2f%% (%d/%d)\n", accuracy, correct, total);

    save_network(net->network, net->nb_layer, "alphabet.nn");
    printf("Réseau sauvegardé dans alphabet.nn\n");

    for (int i = 0; i < total; i++) {
        free(inputs[i]);
        free(outputs[i]);
    }
    free(inputs);
    free(outputs);

    for (int l = 0; l < net->nb_layer; l++) {
        for (int i = 0; i < net->network[l].nb_neurone; i++) free(net->network[l].weights[i]);
        free(net->network[l].weights);
        free(net->network[l].biais);
        free(net->network[l].values);
    }
    free(net->network);
    free(net);

}


//fonction test :
void prediction(char *test_file, char *lettre)
{
        int n_layers;
        struct Layer *layers = load_network("alphabet.nn", &n_layers);

        double *pixels = load_bmp_28x28(test_file);
        

        struct LayerNetwork net_temp={0};
        net_temp.nb_layer = n_layers;
        net_temp.network = layers;

        forward(&net_temp, pixels);
        softmax(net_temp.network[n_layers-1].values, N_CLASSES);
        int pred = argmax(net_temp.network[n_layers-1].values, N_CLASSES);
        printf("→ Prédiction: %c\n", 'A' + pred);

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
}




/*#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "neurone_system.h"
//#include <glib-2.0/glib.h>
#include <math.h>
#include <dirent.h>
#include <string.h>

#define PASSAGE 500000
#define RATE 1


void softmax(double* values, int n) {
    double max = values[0];
    for (int i = 1; i < n; i++) if (values[i] > max) max = values[i];

    double sum = 0;
    for (int i = 0; i < n; i++) {
        values[i] = exp(values[i] - max);
        sum += values[i];
    }
    for (int i = 0; i < n; i++) values[i] /= sum;
}


double* load_bmp_28x28(const char *filename)
{
    FILE *f = fopen(filename, "rb");
    if (!f) { perror("open bmp"); return NULL; }

    unsigned char header[54];
    fread(header, sizeof(unsigned char), 54, f); 

    int width  = *(int*)&header[18];
    int height = *(int*)&header[22];

    if (width != 28 || height != 28) {
        printf("⚠️ L'image doit être 28x28 pixels (actuellement %dx%d)\n", width, height);
        fclose(f);
        return NULL;
    }

    int size = 3 * width * height; 
    unsigned char *data = malloc(size);
    fread(data, sizeof(unsigned char), size, f);
    fclose(f);

    double *pixels = malloc(width * height * sizeof(double));
    for (int i = 0; i < width*height; i++) {
        int r = data[3*i + 2];
        int g = data[3*i + 1];
        int b = data[3*i];
        pixels[i] = (r*0.299 + g*0.587 + b*0.114) / 255.0; 
    }

    free(data);
    return pixels;
}




//INITIALISIZE LAYER
void init_layer(struct Layer *layer, int nb_input, int nb_neurones)
{
	layer->nb_neurone = nb_neurones;
    	layer->nb_entrees = nb_input;

	layer->weights = malloc(nb_neurones * sizeof(double *));
    	layer->biais = malloc(nb_neurones * sizeof(double));
    	layer->values = malloc(nb_neurones * sizeof(double));	
	
	for (int i = 0; i < nb_neurones; i++) {
        	layer->biais[i] = ((double)rand() / RAND_MAX) - 0.5;
        	layer->values[i] = 0.0;
		layer->weights[i] = malloc(nb_input * sizeof(double));
        	for (int j = 0; j < nb_input; j++) {
            		layer->weights[i][j] = ((double)rand() / RAND_MAX) - 0.5;
        	}
    	}
}


//Create a Layer Network
struct LayerNetwork *network(int nb_layers, int* nb_neurones_layers, int nb_neurones_init){
	struct LayerNetwork *N_network = malloc(sizeof(struct LayerNetwork));
	N_network->nb_layer= nb_layers;
	N_network->network = malloc(nb_layers*sizeof(struct Layer));
	
	
	for(int i = 0; i < nb_layers; i++)
	{
		struct Layer *layer = &N_network->network[i];
		if(i == 0)
		{
			init_layer(layer, nb_neurones_init, nb_neurones_layers[i]);
		}
		else{
			init_layer(layer, nb_neurones_layers[i-1], nb_neurones_layers[i]);
		}
	}
	return N_network;
}



double sigmoid(double x) {
    return 1.0 / (1.0 + exp(-x));
}

double dsigmoid(double y) {
    return y * (1.0 - y);
}


//BACKPROPAGATION FUNCTION
void backprop(struct LayerNetwork *net, double *inputs, double target)
{
    int L = net->nb_layer;
    double **delta = malloc(L * sizeof(double*));
    struct Layer *output = &net->network[L - 1];
    delta[L - 1] = malloc(output->nb_neurone * sizeof(double));

    for (int i = 0; i < output->nb_neurone; i++) {
        double erreur = target - output->values[i];
        delta[L - 1][i] = erreur * dsigmoid(output->values[i]);
    }

    for (int l = L - 2; l >= 0; l--) {
        struct Layer *layer = &net->network[l];
        struct Layer *next = &net->network[l + 1];

        delta[l] = malloc(layer->nb_neurone * sizeof(double));
        for (int i = 0; i < layer->nb_neurone; i++) {
            double somme = 0.0;
            for (int j = 0; j < next->nb_neurone; j++)
                somme += next->weights[j][i] * delta[l + 1][j];
            delta[l][i] = somme * dsigmoid(layer->values[i]);
        }
    }
    for (int l = 0; l < L; l++) {
        struct Layer *layer = &net->network[l];
        double *entrees = (l == 0) ? inputs : net->network[l - 1].values;

        for (int i = 0; i < layer->nb_neurone; i++) {
            for (int j = 0; j < layer->nb_entrees; j++)
                layer->weights[i][j] += RATE * delta[l][i] * entrees[j];
            layer->biais[i] += RATE * delta[l][i];
        }
    }
    for (int l = 0; l < L; l++) free(delta[l]);
    free(delta);
}


//FORWARD PROPAGATION FUNCTION
void forward(struct LayerNetwork *net, double *inputs)
{
	for (int l = 0; l < net->nb_layer; l++) {
        struct Layer *layer = &net->network[l];
        double *entrees = (l == 0) ? inputs : net->network[l - 1].values;

        for (int i = 0; i < layer->nb_neurone; i++) {
            double somme = 0.0;
            for (int j = 0; j < layer->nb_entrees; j++) {
                somme += entrees[j] * layer->weights[i][j];
            }
            somme += layer->biais[i];
            layer->values[i] = sigmoid(somme);
        }
    }
}
						

//TRAINING FUNCTION
void traning(struct LayerNetwork *ln, double** inputs, double** outputs, int size_input)
{
    for (int epoch = 0; epoch < PASSAGE; epoch++) {
        for (int i = 0; i < size_input; i++) {
            forward(ln, inputs[i]);
            for (int j = 0; j < ln->network[ln->nb_layer - 1].nb_neurone; j++) {
                double target = outputs[i][j];
                backprop_single_neuron(ln, inputs[i], j, target);
            }
        }
    }
}


//SAVING THE NETWORK TO A FILE TO USE IT 
void save_network(struct Layer *layers, int n_layers, const char *filename) {
    FILE *f = fopen(filename, "wb");
    if (!f) { perror("fopen save"); return; }

    fwrite(&n_layers, sizeof(int), 1, f);
    for (int i = 0; i < n_layers; i++) {
        fwrite(&layers[i].nb_entrees, sizeof(int), 1, f);
        fwrite(&layers[i].nb_neurone, sizeof(int), 1, f);
        for (int ni = 0; ni < layers[i].nb_neurone; ni++) {
            fwrite(layers[i].weights[ni], sizeof(double), layers[i].nb_entrees, f);
        }
        fwrite(layers[i].biais, sizeof(double), layers[i].nb_neurone, f);
    }

    fclose(f);
}

// LOADING THE NETWORK TO USE IT  

struct Layer* load_network(const char *filename, int *n_layers_out) {
    FILE *f = fopen(filename, "rb");
    if (!f) { perror("fopen load"); return NULL; }

    int n_layers;
    if (fread(&n_layers, sizeof(int), 1, f) != 1) { fclose(f); return NULL; }
    *n_layers_out = n_layers;

    struct Layer *layers = malloc(sizeof(struct Layer) * n_layers);
    if (!layers) { fclose(f); return NULL; }

    for (int i = 0; i < n_layers; i++) {
        if (fread(&layers[i].nb_entrees, sizeof(int), 1, f) != 1) { fclose(f); free(layers); return NULL; }
        if (fread(&layers[i].nb_neurone, sizeof(int), 1, f) != 1) { fclose(f); free(layers); return NULL; }
        layers[i].weights = malloc(layers[i].nb_neurone * sizeof(double*));
        if (!layers[i].weights) { perror("malloc weights"); fclose(f); free(layers); return NULL; }

        for (int ni = 0; ni < layers[i].nb_neurone; ni++) {
            layers[i].weights[ni] = malloc(layers[i].nb_entrees * sizeof(double));
            if (!layers[i].weights[ni]) { perror("malloc weights[ni]"); }
            if (fread(layers[i].weights[ni], sizeof(double), layers[i].nb_entrees, f) != (size_t)layers[i].nb_entrees) {
                perror("fread weights");
                fclose(f); return NULL;
            }
        }

        layers[i].biais = malloc(layers[i].nb_neurone * sizeof(double));
        if (!layers[i].biais) { perror("malloc biais"); fclose(f); return NULL; }
        if (fread(layers[i].biais, sizeof(double), layers[i].nb_neurone, f) != (size_t)layers[i].nb_neurone) {
            perror("fread biais");
            fclose(f); return NULL;
        }
        layers[i].values = malloc(layers[i].nb_neurone * sizeof(double));
        for (int v = 0; v < layers[i].nb_neurone; v++) layers[i].values[v] = 0.0;
    }

    fclose(f);
    return layers;
}


//PREDICT THE RESULT REGARDING TO THE NETWORK LOADED

double *prediction(double *input, const char *file_name)
{
    int n_layers;
    struct Layer *final_layer = load_network(file_name, &n_layers);
    double *values = input;
    for (int l = 0; l < n_layers; l++) {
        struct Layer *layer = &final_layer[l];
        double *next_values = malloc(layer->nb_neurone * sizeof(double));

        for (int i = 0; i < layer->nb_neurone; i++) {
            double somme = 0.0;
            for (int j = 0; j < layer->nb_entrees; j++) {
                somme += values[j] * layer->weights[i][j];
            }
            somme += layer->biais[i];
            next_values[i] = sigmoid(somme);
        }
        if (l > 0) free(values);
        values = next_values;
    }

    return values;
}


//FUNTION TO START THE TRAINING AND SAVE IT
void principal(double **inputs, int *neurones_par_couche, const char *file_name, double **outputs, int nb_layers, int nb_inputs)
{
    srand(0); 
    struct LayerNetwork *net = network(nb_layers, neurones_par_couche, 784); 
    traning(net, inputs, outputs, nb_inputs);
    save_network(net->network, net->nb_layer, file_name);

    printf("\n=== Résultat après entraînement ===\n");
    for (int i = 0; i < nb_inputs; i++) {
        forward(net, inputs[i]);
        printf("Image %d -> ", i);
        for (int j = 0; j < 26; j++)
            printf("%.2f ", net->network[net->nb_layer - 1].values[j]);
        printf("\n");
    }
}


int main(int argc, char* argv)
{
    int nb_layers = 3;
    int neurons_per_layer[] = {128, 64, 26}; // 2 couches cachées + sortie
    int nb_inputs = 784;
    double* pixels = load_bmp_28x28(argv[1]);

    char letters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
int nb_total_images = 0;
for (int l = 0; l < 26; l++) {
    char folder[64];
    sprintf(folder, "./%c/", letters[l]);
    DIR* dir = opendir(folder);
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type != DT_REG) continue;
        nb_total_images++;
    }
    closedir(dir);
}

// allouer inputs et outputs après avoir compté
double** inputs = malloc(nb_total_images * sizeof(double*));
double** outputs = malloc(nb_total_images * sizeof(double*));

    int index = 0;

for (int l = 0; l < 26; l++) {
    char folder[64];
    sprintf(folder, "./%c/", letters[l]);
    // Parcourir tous les fichiers BMP dans folder
    DIR* dir = opendir(folder);
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type != DT_REG) continue; // ignorer . et ..
        char path[128];
        sprintf(path, "%s%s", folder, entry->d_name);
        inputs[index] = load_bmp_28x28(path);

        outputs[index] = calloc(26, sizeof(double));
        outputs[index][l] = 1.0; // one-hot
        index++;
    }
    closedir(dir);
}



    return 0;
}*/


	
