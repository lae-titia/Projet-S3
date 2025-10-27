#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
// ici il faut inclure xor.h  #include "neurone_system.h"

// Configuration
#define IMAGE_SIZE 35        // Grille 5x7 pixels
#define HIDDEN_SIZE 20       // Nombre de neurones cachés
#define OUTPUT_SIZE 26       // A-Z
#define LEARNING_RATE 0.1
#define EPOCHS 50000         // Nombre d'itérations d'apprentissage


double sigmoid(double x) {
    return 1.0 / (1.0 + exp(-x));
}

double dsigmoid(double y) {
    return y * (1.0 - y);
}

void softmax(double *values, int size) {
    double max = values[0];
    for (int i = 1; i < size; i++) {
        if (values[i] > max) max = values[i];
    }
    
    double sum = 0.0;
    for (int i = 0; i < size; i++) {
        values[i] = exp(values[i] - max);
        sum += values[i];
    }
    
    for (int i = 0; i < size; i++) {
        values[i] /= sum;
    }
}


void forward_ocr(struct LayerNetwork *net, double *inputs)
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
            
            if (l < net->nb_layer - 1) {
                layer->values[i] = sigmoid(somme);
            } else {
                layer->values[i] = somme;
            }
        }
        
        if (l == net->nb_layer - 1) {
            softmax(layer->values, layer->nb_neurone);
        }
    }
}

void backprop_ocr(struct LayerNetwork *net, double *inputs, double *targets)
{
    int L = net->nb_layer;
    double **delta = malloc(L * sizeof(double*));
    struct Layer *output = &net->network[L - 1];
    
    // Calcul delta sortie
    delta[L - 1] = malloc(output->nb_neurone * sizeof(double));
    for (int i = 0; i < output->nb_neurone; i++) {
        delta[L - 1][i] = output->values[i] - targets[i];
    }
    
    // Backpropagation
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
    
    // Mise à jour des poids
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


// Définition de quelques lettres pour tester (grille 5x7)
double letter_A[35] = {
    0,1,1,1,0,
    1,0,0,0,1,
    1,0,0,0,1,
    1,1,1,1,1,
    1,0,0,0,1,
    1,0,0,0,1,
    1,0,0,0,1
};

double letter_B[35] = {
    1,1,1,1,0,
    1,0,0,0,1,
    1,0,0,0,1,
    1,1,1,1,0,
    1,0,0,0,1,
    1,0,0,0,1,
    1,1,1,1,0
};

double letter_C[35] = {
    0,1,1,1,0,
    1,0,0,0,1,
    1,0,0,0,0,
    1,0,0,0,0,
    1,0,0,0,0,
    1,0,0,0,1,
    0,1,1,1,0
};

double letter_I[35] = {
    1,1,1,1,1,
    0,0,1,0,0,
    0,0,1,0,0,
    0,0,1,0,0,
    0,0,1,0,0,
    0,0,1,0,0,
    1,1,1,1,1
};

double letter_O[35] = {
    0,1,1,1,0,
    1,0,0,0,1,
    1,0,0,0,1,
    1,0,0,0,1,
    1,0,0,0,1,
    1,0,0,0,1,
    0,1,1,1,0
};


// CRÉATION DU DATASET D'ENTRAÎNEMENT

/**
 * Crée les vecteurs one-hot pour les labels
 * Retourne un tableau [26][26] où chaque ligne représente une lettre
 */
double** create_onehot_labels() {
    double **labels = malloc(26 * sizeof(double*));
    
    for (int i = 0; i < 26; i++) {
        labels[i] = calloc(26, sizeof(double));
        labels[i][i] = 1.0;  // One-hot: seulement la position i à 1
    }
    
    return labels;
}

/**
 * Crée un dataset simple avec 5 lettres pour tester
 * Dans votre projet final, vous chargerez toutes les 26 lettres depuis des images
 */
double** create_simple_dataset(int *nb_samples) {
    *nb_samples = 5;  // A, B, C, I, O
    
    double **dataset = malloc(5 * sizeof(double*));
    
    // Allouer et copier chaque lettre
    dataset[0] = malloc(35 * sizeof(double));
    for (int i = 0; i < 35; i++) dataset[0][i] = letter_A[i];
    
    dataset[1] = malloc(35 * sizeof(double));
    for (int i = 0; i < 35; i++) dataset[1][i] = letter_B[i];
    
    dataset[2] = malloc(35 * sizeof(double));
    for (int i = 0; i < 35; i++) dataset[2][i] = letter_C[i];
    
    dataset[8] = malloc(35 * sizeof(double));  // I = 9ème lettre (index 8)
    for (int i = 0; i < 35; i++) dataset[8][i] = letter_I[i];
    
    dataset[14] = malloc(35 * sizeof(double));  // O = 15ème lettre (index 14)
    for (int i = 0; i < 35; i++) dataset[14][i] = letter_O[i];
    
    return dataset;
}

/**
 * Indices des lettres dans le dataset simple
 */
int simple_dataset_indices[5] = {0, 1, 2, 8, 14};  // A, B, C, I, O


// FONCTIONS DE TEST

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
 * Prédit avec affichage des probabilités
 */
char predict_with_confidence(struct LayerNetwork *net, double *image, double *confidence) {
    int index = predict_letter(net, image);
    struct Layer *output = &net->network[net->nb_layer - 1];
    *confidence = output->values[index];
    return 'A' + index;
}

/**
 * Affiche une lettre 5x7
 */
void print_letter(double *letter) {
    for (int row = 0; row < 7; row++) {
        for (int col = 0; col < 5; col++) {
            printf("%c ", letter[row * 5 + col] > 0.5 ? '█' : '.');
        }
        printf("\n");
    }
}

/**
 * Teste le réseau sur toutes les lettres du dataset
 */
void test_network(struct LayerNetwork *net, double **dataset, int *indices, int nb_samples) {
    printf("\n=== TEST DU RÉSEAU ===\n");
    int correct = 0;
    
    for (int i = 0; i < nb_samples; i++) {
        int expected_index = indices[i];
        char expected_letter = 'A' + expected_index;
        
        double confidence;
        char predicted = predict_with_confidence(net, dataset[expected_index], &confidence);
        
        printf("\nLettre attendue: %c\n", expected_letter);
        print_letter(dataset[expected_index]);
        printf("Prédiction: %c (confiance: %.2f%%)\n", 
               predicted, confidence * 100);
        
        if (predicted == expected_letter) {
            printf("✅ CORRECT\n");
            correct++;
        } else {
            printf("❌ ERREUR\n");
        }
    }
    
    printf("\n=== RÉSULTATS ===\n");
    printf("Taux de réussite: %d/%d (%.1f%%)\n", 
           correct, nb_samples, (correct * 100.0) / nb_samples);
}


// FONCTION D'ENTRAÎNEMENT

void train_ocr(struct LayerNetwork *net, double **images, double **labels, 
               int *indices, int nb_samples, int epochs)
{
    printf("\n=== DÉBUT DE L'ENTRAÎNEMENT ===\n");
    printf("Nombre d'époques: %d\n", epochs);
    printf("Nombre d'exemples: %d\n", nb_samples);
    printf("Taux d'apprentissage: %.2f\n\n", LEARNING_RATE);
    
    for (int epoch = 0; epoch < epochs; epoch++) {
        double total_loss = 0.0;
        
        // Entraîner sur tous les exemples
        for (int i = 0; i < nb_samples; i++) {
            int letter_index = indices[i];
            forward_ocr(net, images[letter_index]);
            backprop_ocr(net, images[letter_index], labels[letter_index]);
            
            // Calcul de la perte (cross-entropy)
            struct Layer *output = &net->network[net->nb_layer - 1];
            for (int j = 0; j < OUTPUT_SIZE; j++) {
                if (labels[letter_index][j] == 1.0) {
                    total_loss -= log(output->values[j] + 1e-10);
                }
            }
        }
        
        // Affichage de la progression
        if (epoch % 5000 == 0) {
            double avg_loss = total_loss / nb_samples;
            printf("Époque %5d - Perte moyenne: %.4f\n", epoch, avg_loss);
        }
    }
    
    printf("\n✅ Entraînement terminé !\n");
}

// SAUVEGARDE ET CHARGEMENT

void save_network(struct LayerNetwork *net, const char *filename) {
    FILE *f = fopen(filename, "wb");
    if (!f) {
        fprintf(stderr, "Erreur: impossible d'ouvrir %s en écriture\n", filename);
        return;
    }
    
    fwrite(&net->nb_layer, sizeof(int), 1, f);
    
    for (int l = 0; l < net->nb_layer; l++) {
        struct Layer *layer = &net->network[l];
        fwrite(&layer->nb_neurone, sizeof(int), 1, f);
        fwrite(&layer->nb_entrees, sizeof(int), 1, f);
        
        for (int i = 0; i < layer->nb_neurone; i++) {
            fwrite(layer->weights[i], sizeof(double), layer->nb_entrees, f);
            fwrite(&layer->biais[i], sizeof(double), 1, f);
        }
    }
    
    fclose(f);
    printf("✅ Réseau sauvegardé dans '%s'\n", filename);
}

void load_network(struct LayerNetwork *net, const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) {
        fprintf(stderr, "Erreur: impossible d'ouvrir %s en lecture\n", filename);
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
    printf("✅ Réseau chargé depuis '%s'\n", filename);
}


// PROGRAMME PRINCIPAL


int main(void)
{
    srand(time(NULL));
    
    printf("╔══════════════════════════════════════════════╗\n");
    printf("║   OCR - Reconnaissance de Lettres           ║\n");
    printf("║   Réseau de Neurones A-Z                    ║\n");
    printf("╚══════════════════════════════════════════════╝\n\n");
    
    // ===== 1. CRÉER LE RÉSEAU =====
    printf("Création du réseau de neurones...\n");
    int layers[] = {HIDDEN_SIZE, OUTPUT_SIZE};
    struct LayerNetwork *net = network(2, layers, IMAGE_SIZE);
    printf("   Architecture: %d → %d → %d\n", IMAGE_SIZE, HIDDEN_SIZE, OUTPUT_SIZE);
    printf("    Réseau créé avec succès\n\n");
    
    // ===== 2. PRÉPARER LES DONNÉES =====
    printf(" Préparation des données d'entraînement...\n");
    int nb_samples;
    double **training_images = create_simple_dataset(&nb_samples);
    double **training_labels = create_onehot_labels();
    printf("   Nombre de lettres: %d (A, B, C, I, O)\n", nb_samples);
    printf("    Dataset créé\n\n");
    
    // ===== 3. TEST AVANT ENTRAÎNEMENT =====
    printf("Test AVANT entraînement (le réseau est aléatoire):\n");
    test_network(net, training_images, simple_dataset_indices, nb_samples);
    
    // ===== 4. ENTRAÎNEMENT =====
    printf("\n Lancement de l'entraînement...\n");
    train_ocr(net, training_images, training_labels, 
              simple_dataset_indices, nb_samples, EPOCHS);
    
    // ===== 5. TEST APRÈS ENTRAÎNEMENT =====
    printf("\n Test APRÈS entraînement:\n");
    test_network(net, training_images, simple_dataset_indices, nb_samples);
    
    // ===== 6. SAUVEGARDER LE RÉSEAU =====
    printf("\n Sauvegarde du réseau entraîné...\n");
    save_network(net, "ocr_network.dat");
    
    // ===== 7. EXEMPLE DE CHARGEMENT =====
    printf("\n Test de rechargement du réseau...\n");
    struct LayerNetwork *net2 = network(2, layers, IMAGE_SIZE);
    load_network(net2, "ocr_network.dat");
    printf("\n Test du réseau rechargé:\n");
    test_network(net2, training_images, simple_dataset_indices, nb_samples);
    
    printf("\n Programme terminé avec succès !\n");
    printf("\nFichier généré: ocr_network.dat\n");
    printf("Vous pouvez maintenant utiliser ce réseau pour reconnaître des lettres !\n");
    
    return 0;
}
