#ifndef NEURONE_SYSTEM_H
#define NEURONE_SYSTEM_H

//#include <glib-2.0/glib.h>

struct Layer {
	int nb_neurone;
	int nb_entrees;
	double **weights;
	double *biais;
	double *values;
};

struct LayerNetwork{
	int nb_layer;
	struct Layer *network;
};

void init_layer(struct Layer *layer, int nb_input, int nb_neurones);
struct LayerNetwork *network(int nb_layers, int* nb_neurones_layers, int nb_neurones_init);
double sigmoid(double x);
double dsigmoid(double y);
void backprop(struct LayerNetwork *net, double *inputs, double target);
void forward(struct LayerNetwork *net, double *inputs);
void traning(struct LayerNetwork *ln, double** inputs, double* outputs, int size_input);
void save_network(struct Layer *layers, int n_layers, const char *filename);
struct Layer* load_network(const char *filename, int *n_layers_out);
double *prediction(double *input, const char *file_name);
void principal(double **inputs, int *neurones_par_couche,
              const char *file_name, double *outputs,
              int nb_layers, int nb_inputs);
/*
typedef struct neuron_t
{
	float actv;
	float *out_weights;
	float bias;
	float z;

	float dactv;
	float *dw;
	float dbias;
	float dz;
  
} neuron;

typedef struct layer_t
{
	int num_neu;
	struct neuron_t *neu; 
} layer;

typedef struct network_t
{
	int num_layer;
	struct layer;
}layer;
*/
#endif




