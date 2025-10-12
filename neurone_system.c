#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "neurone_system.h"

#define PASSAGE 100000
#define RATE 0.5


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
						
	

void traning(struct LayerNetwork *ln, double** inputs, double* outputs, int size_input)
{
	for (int epoch = 0; epoch < PASSAGE; epoch++) {
        	double loss = 0.0;
		
		for (int i = 0; i < size_input; i++) {
            forward(ln, inputs[i]);
            backprop(ln, inputs[i], outputs[i]);
        }


	}
}


int main(void)
{
    srand(time(NULL));

   int neurones_par_couche[] = {2, 2, 1}; 
    struct LayerNetwork *net = network(3, neurones_par_couche, 2); 

    double **inputs = malloc(4 * sizeof(double*)); 
    for (int i = 0; i < 4; i++) 
    inputs[i] = malloc(2 * sizeof(double)); 
    inputs[0][0] = 0; inputs[0][1] = 0; 
    inputs[1][0] = 0; inputs[1][1] = 1; 
    inputs[2][0] = 1; inputs[2][1] = 0; 
    inputs[3][0] = 1; inputs[3][1] = 1; 
    double *outputs = malloc(4 * sizeof(double)); 
    outputs[0] = 0; 
    outputs[1] = 1; 
    outputs[2] = 1; 
    outputs[3] = 0; 
    traning(net, inputs, outputs, 4);
    /*printf("\n=== Résultats du réseau après entraînement ===\n");
for (int i = 0; i < 4; i++) {
    forward(net, inputs[i]);

    double result = net->network[net->nb_layer - 1].values[0];

    printf("[%g, %g] -> %.3f\n", inputs[i][0], inputs[i][1], result);
}*/

	return 0;
}


	
