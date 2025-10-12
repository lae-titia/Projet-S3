#ifndef NEURONE_SYSTEM_H
#define NEURONE_SYSTEM_H



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


#endif




