/*
 * neural_network.h
 * Header pour le réseau de neurones
 * Projet OCR Word Search Solver
 */

#ifndef NEURAL_NETWORK_H
#define NEURAL_NETWORK_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// ============================================================================
// STRUCTURES
// ============================================================================

/**
 * Structure représentant une couche (layer) du réseau
 */
struct Layer {
    int nb_neurone;        // Nombre de neurones dans cette couche
    int nb_entrees;        // Nombre d'entrées pour chaque neurone
    double **weights;      // Matrice des poids [nb_neurone][nb_entrees]
    double *biais;         // Biais de chaque neurone [nb_neurone]
    double *values;        // Valeurs de sortie de chaque neurone [nb_neurone]
};

/**
 * Structure représentant le réseau de neurones complet
 */
struct LayerNetwork {
    int nb_layer;          // Nombre de couches (sans compter l'entrée)
    struct Layer *network; // Tableau de couches
};

// ============================================================================
// DÉCLARATIONS DE FONCTIONS
// ============================================================================

/**
 * Initialise une couche avec des poids et biais aléatoires
 * 
 * @param layer Pointeur vers la couche à initialiser
 * @param nb_input Nombre d'entrées pour chaque neurone
 * @param nb_neurones Nombre de neurones dans la couche
 */
void init_layer(struct Layer *layer, int nb_input, int nb_neurones);

/**
 * Crée un réseau de neurones complet
 * 
 * @param nb_layers Nombre de couches (hors entrée)
 * @param nb_neurones_layers Tableau contenant le nombre de neurones par couche
 * @param nb_neurones_init Nombre de neurones d'entrée
 * @return Pointeur vers le réseau créé
 */
struct LayerNetwork *network(int nb_layers, int* nb_neurones_layers, int nb_neurones_init);

/**
 * Libère la mémoire allouée pour une couche
 * 
 * @param layer Pointeur vers la couche à libérer
 */
void free_layer(struct Layer *layer);

/**
 * Libère la mémoire allouée pour le réseau complet
 * 
 * @param net Pointeur vers le réseau à libérer
 */
void free_network(struct LayerNetwork *net);

#endif /* NEURAL_NETWORK_H */
