
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "neurone_system.h"

//ATTENTION !!!! ici il faudra aapter selon la taille réelle des images de lettres
//Notes pour moi même: 
//Couche d'entrée= pixels (contient 35 valeurs (5*7 pixels) 0 ou 1 pour noir/blanc) -> ne fait aucun calcul
//Couche cachée= cerveau réseau: détecte différents patterns (barre, courbe...) dans chaque neurone est caché une donnée spécifique -> calculs effectués= somme (entree*poids)+biais, valeur proba entre 0 et 1 = intensité de la caractéristique détectée. Si le neurone s'active fortement c'est qu'il y a bcp de similitudes avec caractère 
//Couche sortie: en quelques sortes c'est le jury -> il décide si la chance est "suffisante" pour que ce soit un caractère (selon proba calculées au dessus)
#define IMAGE_WIDTH 28    // À ajuster selon grille
#define IMAGE_HEIGHT 28   // À ajuster selon grille
#define INPUT_SIZE (IMAGE_WIDTH * IMAGE_HEIGHT)  // 784 pour 28x28
#define OUTPUT_SIZE 26    // A-Z (26 lettres majuscules)
#define HIDDEN_SIZE 128   // Couche cachée (ajustable)
#define LEARNING_RATE 0.1
#define EPOCHS 10000

/**
 * Fonction softmax: remplace sigmoid sur la couche de sortie (le sigmoid prend n'importe quel nb et le transforme en nb entre 0 et 1 
 * Sigmoid permet d'interpréter le chiffre comme une proba 
 * Ici on choisit d'utiliser softmax plutot que sigmo car sigmo analyse chaque lettre indépendamment à chaque neurone et renvoie la proba que ce soit sa lettre. Softmax calcule tous les neurones ensemble: la somme est exactement 1.0 chaque val de neurone dépend des autres
 * appliquée sur la couche de sortie 
 */
void softmax(double *values, int size) {
    double max = values[0];
    for (int i = 1; i < size; i++) {
        if (values[i] > max) max = values[i];
    }
    
    double sum = 0.0;
    for (int i = 0; i < size; i++) {
        values[i] = exp(values[i] - max);  // Soustraire max pour stabilité numérique
        sum += values[i];
    }
    
    for (int i = 0; i < size; i++) {
        values[i] /= sum;
    }
}


//Adapte la fonction forward pour utiliser softmax
//Pour faire passer données de l'entrée vers la sortie du réseau->traverse couches, transforme image lettres en 26 proba (lettres de a à z)
void forward_ocr(struct LayerNetwork *net, double *inputs)
{
    for (int l = 0; l < net->nb_layer; l++) { //pour parcourir couches
        struct Layer *layer = &net->network[l];
        double *entrees = (l == 0) ? inputs : net->network[l - 1].values;//couche cachée l=0 couche sortie l=1
        
        for (int i = 0; i < layer->nb_neurone; i++) {
            double somme = 0.0;
            for (int j = 0; j < layer->nb_entrees; j++) {
                somme += entrees[j] * layer->weights[i][j];
            }
            somme += layer->biais[i];
            
            // Appliquer sigmoid sur les couches cachées
            if (l < net->nb_layer - 1) {
                layer->values[i] = sigmoid(somme);
            } else {
                // Stocker la valeur pour softmax
                layer->values[i] = somme;
            }
        }
        
        // Appliquer softmax uniquement sur la dernière couche
        if (l == net->nb_layer - 1) {
            softmax(layer->values, layer->nb_neurone);
        }
    }
}


//Adapter backprop pour plusieurs sorties + softmax
//but: réseau apprend des erreurs -> calcule erreur commise puis ajuste les poids pour réduire erreurs 
//si réseau predit B alors que c'est A, backprop modifie le poids pour qu'il se rapproche de A
//forward va d'entrée vers sortie et backprop va de sortie vers entrée 

void backprop_ocr(struct LayerNetwork *net, double *inputs, double *targets)
{
    int L = net->nb_layer;
    double **delta = malloc(L * sizeof(double*)); //si delta<0 il devrait s'activer + alors que si delta<0 il devrait d'activer -  -> prediction-attendu
    struct Layer *output = &net->network[L - 1];
    
    // Calcul du delta pour la couche de sortie (avec softmax)
    delta[L - 1] = malloc(output->nb_neurone * sizeof(double));
    for (int i = 0; i < output->nb_neurone; i++) {
        // Pour softmax + cross-entropy, le gradient est simplifié
        delta[L - 1][i] = output->values[i] - targets[i];
    }
    
    // Rétropropagation pour les couches cachées
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
    
    // Mise à jour des poids et biais
    for (int l = 0; l < L; l++) {
        struct Layer *layer = &net->network[l];
        double *entrees = (l == 0) ? inputs : net->network[l - 1].values;
        
        for (int i = 0; i < layer->nb_neurone; i++) {
            for (int j = 0; j < layer->nb_entrees; j++)
                layer->weights[i][j] -= LEARNING_RATE * delta[l][i] * entrees[j];
            layer->biais[i] -= LEARNING_RATE * delta[l][i];
        }
    }
    
    for (int l = 0; l < L; l++) free(delta[l]);
    free(delta);
}


// MODIFICATION 4: Créer les vecteurs one-hot pour les 26 lettres

/**
 * Crée un tableau de vecteurs one-hot pour A-Z
 * Exemple: A = [1,0,0,...,0], B = [0,1,0,...,0], etc.
 */
double** create_onehot_labels() {
    double **labels = malloc(26 * sizeof(double*));
    
    for (int i = 0; i < 26; i++) {
        labels[i] = calloc(26, sizeof(double));  // Initialise à 0
        labels[i][i] = 1.0;  // Met 1 à la position de la lettre
    }
    
    return labels;
}


//Fonction d'entraînement adaptée


void train_ocr(struct LayerNetwork *net, double **images, double **labels, 
               int nb_samples, int epochs)
{
    for (int epoch = 0; epoch < epochs; epoch++) {
        double total_loss = 0.0;
        
        for (int i = 0; i < nb_samples; i++) {
            forward_ocr(net, images[i]);
            backprop_ocr(net, images[i], labels[i]);
            
            // Calcul de la perte (cross-entropy)
            struct Layer *output = &net->network[net->nb_layer - 1];
            for (int j = 0; j < OUTPUT_SIZE; j++) {
                if (labels[i][j] == 1.0) {
                    total_loss -= log(output->values[j] + 1e-10);
                }
            }
        }
        
        // Affichage de la progression
        if (epoch % 1000 == 0) {
            printf("Epoch %d - Loss: %.4f\n", epoch, total_loss / nb_samples);
        }
    }
}


//Fonction de prédiction
/**
 * Prédit la lettre à partir d'une image
 * Retourne l'indice (0=A, 1=B, ..., 25=Z)
 */
int predict_letter(struct LayerNetwork *net, double *image) {
    forward_ocr(net, image);
    
    struct Layer *output = &net->network[net->nb_layer - 1];
    int max_index = 0;
    double max_prob = output->values[0];
    
    for (int i = 1; i < OUTPUT_SIZE; i++) {
        if (output->values[i] > max_prob) {
            max_prob = output->values[i];
            max_index = i;
        }
    }
    
    return max_index;
}

/**
 * Prédit et affiche avec la probabilité
 */
char predict_letter_with_confidence(struct LayerNetwork *net, double *image, 
                                     double *confidence) {
    int index = predict_letter(net, image);
    struct Layer *output = &net->network[net->nb_layer - 1];
    *confidence = output->values[index];
    return 'A' + index;
}

// ============================================================================
// MODIFICATION 7: Chargement d'une image (à implémenter avec votre découpage)
// ============================================================================

/**
 * Convertit une image en niveau de gris en vecteur d'entrée normalisé
 * pixels: tableau WIDTH x HEIGHT de valeurs [0-255]
 * Retourne un tableau de doubles [0.0-1.0]
 */
double* image_to_input(unsigned char *pixels, int width, int height) {
    double *input = malloc(width * height * sizeof(double));
    
    for (int i = 0; i < width * height; i++) {
        input[i] = pixels[i] / 255.0;  // Normalisation [0-1]
    }
    
    return input;
}

// ============================================================================
// MODIFICATION 8: Sauvegarde et chargement du réseau entraîné
// ============================================================================

/**
 * Sauvegarde les poids du réseau dans un fichier
 */
void save_network(struct LayerNetwork *net, const char *filename) {
    FILE *f = fopen(filename, "wb");
    if (!f) {
        fprintf(stderr, "Erreur: impossible d'ouvrir %s\n", filename);
        return;
    }
    
    // Sauvegarder la structure
    fwrite(&net->nb_layer, sizeof(int), 1, f);
    
    for (int l = 0; l < net->nb_layer; l++) {
        struct Layer *layer = &net->network[l];
        fwrite(&layer->nb_neurone, sizeof(int), 1, f);
        fwrite(&layer->nb_entrees, sizeof(int), 1, f);
        
        // Sauvegarder poids et biais
        for (int i = 0; i < layer->nb_neurone; i++) {
            fwrite(layer->weights[i], sizeof(double), layer->nb_entrees, f);
            fwrite(&layer->biais[i], sizeof(double), 1, f);
        }
    }
    
    fclose(f);
    printf("Réseau sauvegardé dans %s\n", filename);
}

/**
 * Charge les poids du réseau depuis un fichier
 */
void load_network(struct LayerNetwork *net, const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) {
        fprintf(stderr, "Erreur: impossible d'ouvrir %s\n", filename);
        return;
    }
    
    int nb_layer;
    fread(&nb_layer, sizeof(int), 1, f);
    
    if (nb_layer != net->nb_layer) {
        fprintf(stderr, "Erreur: structure du réseau incompatible\n");
        fclose(f);
        return;
    }
    
    for (int l = 0; l < net->nb_layer; l++) {
        struct Layer *layer = &net->network[l];
        int nb_neurone, nb_entrees;
        fread(&nb_neurone, sizeof(int), 1, f);
        fread(&nb_entrees, sizeof(int), 1, f);
        
        if (nb_neurone != layer->nb_neurone || nb_entrees != layer->nb_entrees) {
            fprintf(stderr, "Erreur: structure de la couche %d incompatible\n", l);
            fclose(f);
            return;
        }
        
        for (int i = 0; i < layer->nb_neurone; i++) {
            fread(layer->weights[i], sizeof(double), layer->nb_entrees, f);
            fread(&layer->biais[i], sizeof(double), 1, f);
        }
    }
    
    fclose(f);
    printf("Réseau chargé depuis %s\n", filename);
}


// EXEMPLE D'UTILISATION PRINCIPALE

int main(void)
{
    srand(time(NULL));
    
    // Créer le réseau: INPUT_SIZE -> HIDDEN_SIZE -> OUTPUT_SIZE
    int layers[] = {HIDDEN_SIZE, OUTPUT_SIZE};
    struct LayerNetwork *net = network(2, layers, INPUT_SIZE);
    
    printf("Réseau créé: %d -> %d -> %d\n", INPUT_SIZE, HIDDEN_SIZE, OUTPUT_SIZE);
    
    // TODO: Charger vos images d'entraînement
    // double **training_images = load_training_images();
    // double **training_labels = create_onehot_labels();
    
    // Exemple avec des données factices
    printf("\n=== PHASE D'ENTRAÎNEMENT ===\n");
    // train_ocr(net, training_images, training_labels, 26, EPOCHS);
    
    // Sauvegarder le réseau entraîné
    // save_network(net, "ocr_network.dat");
    
    // Pour utiliser un réseau déjà entraîné:
    // load_network(net, "ocr_network.dat");
    
    // Test de reconnaissance
    printf("\n=== PHASE DE TEST ===\n");
    // double *test_image = load_image("letter_A.png");
    // double confidence;
    // char predicted = predict_letter_with_confidence(net, test_image, &confidence);
    // printf("Lettre prédite: %c (confiance: %.2f%%)\n", predicted, confidence * 100);
    
    return 0;
}
