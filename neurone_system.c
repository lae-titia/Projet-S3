#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "neurone_system.h"
#include <glib-2.0/glib.h>

#define PASSAGE 500000
#define RATE 1



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


void principal(double **inputs, int *neurones_par_couche,const gchar *file_name, double *outputs, int nb_layers, int nb_inputs)
{
    printf("c'est dedans");
    srand(0); //mouais 
    struct LayerNetwork *net = network(nb_layers, neurones_par_couche, 2); 
    for(int j = 0; j < nb_inputs; j++)
    {
        printf("[%g, %g]", inputs[j][0], inputs[j][1]);
    }
    traning(net, inputs, outputs, nb_inputs);
    save_network(net->network, net->nb_layer, file_name);
    printf("\n=== Résultats du réseau après entraînement ===\n");
    for (int i = 0; i < nb_inputs; i++) {
    forward(net, inputs[i]);
    double result = net->network[net->nb_layer - 1].values[0];

    printf("[%g, %g] -> %.3f\n", inputs[i][0], inputs[i][1], result);

}
}

/*

int main(void)
{
    srand(time(NULL));

   int neurones_par_couche[] = {2, 2, 1}; 
   //struct LayerNetwork *net = network(3, neurones_par_couche, 2); 

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
   principal(inputs, neurones_par_couche, "xor", outputs, 3, 4);
   // traning(net, inputs, outputs, 4);
    //save_network(net->network, net->nb_layer, "");
    int n_layers;
    struct Layer *final_layer = load_network("xor", &n_layers);
    for (int i = 0; i < n_layers; i++) 
    {printf("Layer %d -> entrees=%d, neurones=%d\n", i, final_layer[i].nb_entrees, final_layer[i].nb_neurone);

    for (int ni = 0; ni < final_layer[i].nb_neurone; ni++) {          // neurone index
        for (int in = 0; in < final_layer[i].nb_entrees; in++) {      // input index
            printf("Poids du neurone [%d] input[%d] : %f\n", ni, in, final_layer[i].weights[ni][in]);
        }
    }
    for (int k = 0; k < final_layer[i].nb_neurone; k++) {
        printf("Bias du noeud %d : %f\n", k, final_layer[i].biais[k]);
    }}
    double input1[2] = {0, 0};
double input2[2] = {0, 1};
double input3[2] = {1, 0};
double input4[2] = {1, 1};

double *out1 = prediction(final_layer, n_layers, input1);
double *out2 = prediction(final_layer, n_layers, input2);
double *out3 = prediction(final_layer, n_layers, input3);
double *out4 = prediction(final_layer, n_layers, input4);

printf("Résultat (0,0) = %.5f\n", out1[0]);
printf("Résultat (0,1) = %.5f\n", out2[0]);
printf("Résultat (1,0) = %.5f\n", out3[0]);
printf("Résultat (1,1) = %.5f\n", out4[0]);

free(out1);
free(out2);
free(out3);
free(out4);






	return 0;

}


	*/
