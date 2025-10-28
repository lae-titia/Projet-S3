/*
 * neurone_system.c
 * Implémentation du système de réseau de neurones
 */

#include "neural_network.h"

// ============================================================================
// IMPLÉMENTATION DES FONCTIONS
// ============================================================================

void init_layer(struct Layer *layer, int nb_input, int nb_neurones)
{
    layer->nb_neurone = nb_neurones;
    layer->nb_entrees = nb_input;
    
    // Allouer les poids, biais et valeurs
    layer->weights = malloc(nb_neurones * sizeof(double *));
    layer->biais = malloc(nb_neurones * sizeof(double));
    layer->values = malloc(nb_neurones * sizeof(double));
    
    // Initialiser chaque neurone
    for (int i = 0; i < nb_neurones; i++) {
        // Initialisation aléatoire des biais entre -0.5 et 0.5
        layer->biais[i] = ((double)rand() / RAND_MAX) - 0.5;
        
        // Initialiser les valeurs à 0
        layer->values[i] = 0.0;
        
        // Allouer et initialiser les poids
        layer->weights[i] = malloc(nb_input * sizeof(double));
        for (int j = 0; j < nb_input; j++) {
            // Initialisation aléatoire des poids entre -0.5 et 0.5
            layer->weights[i][j] = ((double)rand() / RAND_MAX) - 0.5;
        }
    }
}

struct LayerNetwork *network(int nb_layers, int* nb_neurones_layers, int nb_neurones_init)
{
    // Allouer la structure du réseau
    struct LayerNetwork *N_network = malloc(sizeof(struct LayerNetwork));
    N_network->nb_layer = nb_layers;
    N_network->network = malloc(nb_layers * sizeof(struct Layer));
    
    // Créer chaque couche
    for (int i = 0; i < nb_layers; i++) {
        struct Layer *layer = &N_network->network[i];
        
        if (i == 0) {
            // Première couche : connectée à l'entrée
            init_layer(layer, nb_neurones_init, nb_neurones_layers[i]);
        } else {
            // Couches suivantes : connectées à la couche précédente
            init_layer(layer, nb_neurones_layers[i - 1], nb_neurones_layers[i]);
        }
    }
    
    return N_network;
}

void free_layer(struct Layer *layer)
{
    // Libérer les poids de chaque neurone
    for (int i = 0; i < layer->nb_neurone; i++) {
        free(layer->weights[i]);
    }
    
    // Libérer les tableaux principaux
    free(layer->weights);
    free(layer->biais);
    free(layer->values);
}

void free_network(struct LayerNetwork *net)
{
    // Libérer chaque couche
    for (int i = 0; i < net->nb_layer; i++) {
        free_layer(&net->network[i]);
    }
    
    // Libérer le tableau de couches
    free(net->network);
    
    // Libérer la structure du réseau
    free(net);
}
