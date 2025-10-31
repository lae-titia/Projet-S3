#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "neurone_system.h"
//#include <glib-2.0/glib.h>

#define PASSAGE 500000
#define RATE 1


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
void traning(struct LayerNetwork *ln, double** inputs, double* outputs, int size_input)
{
	for (int epoch = 0; epoch < PASSAGE; epoch++) {
		
		for (int i = 0; i < size_input; i++) {
            forward(ln, inputs[i]);
            backprop(ln, inputs[i], outputs[i]);
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
void principal(double **inputs, int *neurones_par_couche,const char *file_name, double *outputs, int nb_layers, int nb_inputs)
{
    srand(0); 
    struct LayerNetwork *net = network(nb_layers, neurones_par_couche, 2); 
    traning(net, inputs, outputs, nb_inputs);
    save_network(net->network, net->nb_layer, file_name);
    printf("\n=== Result after training ===\n");
    for (int i = 0; i < nb_inputs; i++) {
    forward(net, inputs[i]);
    double result = net->network[net->nb_layer - 1].values[0];

    printf("[%g, %g] -> %.3f\n", inputs[i][0], inputs[i][1], result);

}
}


	
