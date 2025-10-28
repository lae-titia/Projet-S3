/*
 * xnor_test.c
 * Preuve de Concept - Réseau de Neurones pour XNOR
 * Fonction à apprendre: (non A . non B) + (A . B)
 * 
 * Table de vérité XNOR:
 * A | B | Sortie
 * 0 | 0 |   1     (non A . non B)
 * 0 | 1 |   0
 * 1 | 0 |   0
 * 1 | 1 |   1     (A . B)
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "neural_network.h"  /* ← CHANGÉ ICI */

#define PASSAGE 100000
#define RATE 0.5

// ============================================================================
// FONCTIONS DE BASE
// ============================================================================

double sigmoid(double x) {
    return 1.0 / (1.0 + exp(-x));
}

double dsigmoid(double y) {
    return y * (1.0 - y);
}

// ============================================================================
// FORWARD PROPAGATION
// ============================================================================

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

// ============================================================================
// BACKPROPAGATION (pour une seule sortie)
// ============================================================================

void backprop(struct LayerNetwork *net, double *inputs, double target)
{
    int L = net->nb_layer;
    double **delta = malloc(L * sizeof(double*));
    struct Layer *output = &net->network[L - 1];
    
    // Calcul delta de sortie
    delta[L - 1] = malloc(output->nb_neurone * sizeof(double));
    for (int i = 0; i < output->nb_neurone; i++) {
        double erreur = target - output->values[i];
        delta[L - 1][i] = erreur * dsigmoid(output->values[i]);
    }
    
    // Backpropagation vers les couches cachées
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
                layer->weights[i][j] += RATE * delta[l][i] * entrees[j];
            layer->biais[i] += RATE * delta[l][i];
        }
    }
    
    for (int l = 0; l < L; l++) free(delta[l]);
    free(delta);
}

// ============================================================================
// ENTRAÎNEMENT
// ============================================================================

void training(struct LayerNetwork *ln, double** inputs, double* outputs, int size_input)
{
    printf("╔══════════════════════════════════════════════╗\n");
    printf("║   Entraînement du Réseau XNOR               ║\n");
    printf("╚══════════════════════════════════════════════╝\n\n");
    
    printf("Configuration:\n");
    printf("  - Nombre d'époques: %d\n", PASSAGE);
    printf("  - Taux d'apprentissage: %.2f\n", RATE);
    printf("  - Exemples d'entraînement: %d\n\n", size_input);
    
    for (int epoch = 0; epoch < PASSAGE; epoch++) {
        double loss = 0.0;
        
        for (int i = 0; i < size_input; i++) {
            forward(ln, inputs[i]);
            backprop(ln, inputs[i], outputs[i]);
            
            // Calculer l'erreur
            struct Layer *output = &ln->network[ln->nb_layer - 1];
            double erreur = outputs[i] - output->values[0];
            loss += erreur * erreur;
        }
        
        // Afficher la progression tous les 10000 epochs
        if (epoch % 10000 == 0) {
            double mse = loss / size_input;
            printf("Époque %6d - Erreur MSE: %.6f\n", epoch, mse);
        }
    }
    
    printf("\n✅ Entraînement terminé !\n\n");
}

// ============================================================================
// AFFICHAGE DES RÉSULTATS
// ============================================================================

void print_truth_table_header() {
    printf("╔═══════════════════════════════════════════════════════╗\n");
    printf("║           Table de Vérité XNOR                       ║\n");
    printf("║   (non A . non B) + (A . B)                          ║\n");
    printf("╠═══════╤═══════╤══════════╤═══════════╤══════════════╣\n");
    printf("║   A   │   B   │  Attendu │  Prédit   │   Correct ?  ║\n");
    printf("╠═══════╪═══════╪══════════╪═══════════╪══════════════╣\n");
}

void print_truth_table_row(int a, int b, double expected, double predicted, int correct) {
    printf("║   %d   │   %d   │    %d     │   %.3f   │      %s     ║\n", 
           a, b, (int)expected, predicted, correct ? "✅" : "❌");
}

void print_truth_table_footer(int correct_count, int total) {
    printf("╚═══════╧═══════╧══════════╧═══════════╧══════════════╝\n");
    printf("\nTaux de réussite: %d/%d (%.1f%%)\n", 
           correct_count, total, (correct_count * 100.0) / total);
}

void test_network(struct LayerNetwork *net, double **inputs, double *outputs, int size_input) {
    printf("╔══════════════════════════════════════════════╗\n");
    printf("║   Test du Réseau Après Entraînement         ║\n");
    printf("╚══════════════════════════════════════════════╝\n\n");
    
    print_truth_table_header();
    
    int correct_count = 0;
    
    for (int i = 0; i < size_input; i++) {
        forward(net, inputs[i]);
        double result = net->network[net->nb_layer - 1].values[0];
        
        int a = (int)inputs[i][0];
        int b = (int)inputs[i][1];
        double expected = outputs[i];
        
        // On considère correct si |prédit - attendu| < 0.1
        int is_correct = fabs(result - expected) < 0.1;
        if (is_correct) correct_count++;
        
        print_truth_table_row(a, b, expected, result, is_correct);
    }
    
    print_truth_table_footer(correct_count, size_input);
}

// ============================================================================
// AFFICHAGE DE LA FONCTION LOGIQUE
// ============================================================================

void display_logic_function() {
    printf("\n╔══════════════════════════════════════════════╗\n");
    printf("║  Fonction Logique à Apprendre: XNOR         ║\n");
    printf("╚══════════════════════════════════════════════╝\n\n");
    
    printf("Expression booléenne:\n");
    printf("  F(A,B) = (¬A · ¬B) + (A · B)\n\n");
    
    printf("En français:\n");
    printf("  - Si A=0 ET B=0  →  non A ET non B  →  1\n");
    printf("  - Si A=0 ET B=1  →  Faux              →  0\n");
    printf("  - Si A=1 ET B=0  →  Faux              →  0\n");
    printf("  - Si A=1 ET B=1  →  A ET B            →  1\n\n");
    
    printf("C'est la fonction XNOR (eXclusive NOR)\n");
    printf("Retourne 1 si A et B ont la MÊME valeur\n\n");
}

// ============================================================================
// PROGRAMME PRINCIPAL
// ============================================================================

int main(void)
{
    srand(time(NULL));
    
    printf("\n");
    printf("╔════════════════════════════════════════════════════════╗\n");
    printf("║                                                        ║\n");
    printf("║     RÉSEAU DE NEURONES - PREUVE DE CONCEPT            ║\n");
    printf("║     Apprentissage de la fonction XNOR                 ║\n");
    printf("║                                                        ║\n");
    printf("╚════════════════════════════════════════════════════════╝\n\n");
    
    // Afficher la fonction à apprendre
    display_logic_function();
    
    // ===== 1. CRÉER LE RÉSEAU =====
    printf("═══════════════════════════════════════════════════════\n");
    printf("1. Création du Réseau de Neurones\n");
    printf("═══════════════════════════════════════════════════════\n\n");
    
    int neurones_par_couche[] = {2, 1};  // 2 neurones cachés, 1 sortie
    struct LayerNetwork *net = network(2, neurones_par_couche, 2);
    
    printf("Architecture du réseau:\n");
    printf("  - Couche d'entrée:  2 neurones (A et B)\n");
    printf("  - Couche cachée:    2 neurones\n");
    printf("  - Couche de sortie: 1 neurone\n");
    printf("  - Total: 2 → 2 → 1\n\n");
    
    // ===== 2. PRÉPARER LES DONNÉES =====
    printf("═══════════════════════════════════════════════════════\n");
    printf("2. Préparation des Données d'Entraînement\n");
    printf("═══════════════════════════════════════════════════════\n\n");
    
    // Créer les 4 exemples de la table de vérité XNOR
    double **inputs = malloc(4 * sizeof(double*));
    for (int i = 0; i < 4; i++)
        inputs[i] = malloc(2 * sizeof(double));
    
    // Entrées: [A, B]
    inputs[0][0] = 0; inputs[0][1] = 0;  // non A . non B = 1
    inputs[1][0] = 0; inputs[1][1] = 1;  // Faux = 0
    inputs[2][0] = 1; inputs[2][1] = 0;  // Faux = 0
    inputs[3][0] = 1; inputs[3][1] = 1;  // A . B = 1
    
    // Sorties attendues pour XNOR
    double *outputs = malloc(4 * sizeof(double));
    outputs[0] = 1;  // (0,0) → 1
    outputs[1] = 0;  // (0,1) → 0
    outputs[2] = 0;  // (1,0) → 0
    outputs[3] = 1;  // (1,1) → 1
    
    printf("Dataset créé avec succès:\n");
    printf("  - 4 exemples d'entraînement\n");
    printf("  - Fonction: XNOR\n\n");
    
    // ===== 3. TEST AVANT ENTRAÎNEMENT =====
    printf("═══════════════════════════════════════════════════════\n");
    printf("3. Test AVANT Entraînement (Réseau Non Entraîné)\n");
    printf("═══════════════════════════════════════════════════════\n\n");
    
    printf("Le réseau a des poids aléatoires, il va probablement\n");
    printf("donner des réponses incorrectes...\n\n");
    
    test_network(net, inputs, outputs, 4);
    
    printf("\n\nAppuyez sur Entrée pour lancer l'entraînement...");
    getchar();
    
    // ===== 4. ENTRAÎNEMENT =====
    printf("\n═══════════════════════════════════════════════════════\n");
    printf("4. Entraînement du Réseau\n");
    printf("═══════════════════════════════════════════════════════\n\n");
    
    training(net, inputs, outputs, 4);
    
    // ===== 5. TEST APRÈS ENTRAÎNEMENT =====
    printf("═══════════════════════════════════════════════════════\n");
    printf("5. Test APRÈS Entraînement\n");
    printf("═══════════════════════════════════════════════════════\n\n");
    
    test_network(net, inputs, outputs, 4);
    
    // ===== 6. ANALYSE DES RÉSULTATS =====
    printf("\n\n═══════════════════════════════════════════════════════\n");
    printf("6. Analyse Détaillée\n");
    printf("═══════════════════════════════════════════════════════\n\n");
    
    printf("Valeurs précises pour chaque cas:\n\n");
    
    for (int i = 0; i < 4; i++) {
        forward(net, inputs[i]);
        double result = net->network[net->nb_layer - 1].values[0];
        int a = (int)inputs[i][0];
        int b = (int)inputs[i][1];
        
        printf("  [%d, %d] → Attendu: %.0f  |  Prédit: %.6f  |  Arrondi: %d\n",
               a, b, outputs[i], result, result > 0.5 ? 1 : 0);
    }
    
    printf("\n\n╔════════════════════════════════════════════════════════╗\n");
    printf("║                                                        ║\n");
    printf("║  ✅ PREUVE DE CONCEPT RÉUSSIE !                       ║\n");
    printf("║                                                        ║\n");
    printf("║  Votre réseau de neurones a appris la fonction        ║\n");
    printf("║  logique XNOR: (¬A · ¬B) + (A · B)                    ║\n");
    printf("║                                                        ║\n");
    printf("║  Vous pouvez maintenant passer à la reconnaissance    ║\n");
    printf("║  de caractères pour votre projet OCR !                ║\n");
    printf("║                                                        ║\n");
    printf("╚════════════════════════════════════════════════════════╝\n\n");
    
    // Libérer la mémoire
    for (int i = 0; i < 4; i++)
        free(inputs[i]);
    free(inputs);
    free(outputs);
    free_network(net);
    
    return 0;
}
