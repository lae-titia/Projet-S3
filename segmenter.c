#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib-2.0/glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk/gdk.h>
#include <sys/stat.h>
#include <errno.h> // Pour EEXIST
#include <math.h>  // Pour Sobel (sqrt, atan2)
#include <unistd.h> // Pour stat

// --- PARAMÈTES À AJUSTER ---

// Seuil pour la séparation initiale Liste/Grille (nb pixels min. pour un bloc)
#define MAIN_SEPARATOR_THRESHOLD 5
// Seuil de bruit pour trouver les lignes de mots (nb pixels min. par ligne)
#define LIST_HORIZ_NOISE_THRESHOLD 1

// --- Constantes pour la Liste de Mots (Logique CCA) ---
// Ratio (largeur/hauteur) au-dessus duquel un blob est "large" (ex: "PA")
#define LETTER_ASPECT_RATIO_THRESHOLD 1.636 
// Facteur de "creux" pour couper les blobs larges (ex: 0.45 = 45% du pic)
#define SPLIT_VALLEY_FACTOR 0.45 

// --- Constantes pour la Grille (Logique Hough) ---
// Tolérance d'angle pour la détection de lignes (ex: 5.0 = +/- 5 degrés)
#define HOUGH_ANGLE_TOLERANCE 5.0
// Seuil de magnitude de gradient (Sobel) pour qu'un pixel soit un "contour"
#define SOBEL_EDGE_THRESHOLD 220
// Seuil de "pic" (nb de pixels de contour) pour détecter une ligne de grille
#define HOUGH_PEAK_THRESHOLD 120
// Distance (pixels) pour fusionner deux lignes de grille détectées en double
#define LINE_GROUPING_DISTANCE 25
// Seuil de taille (ex: 0.85 = 85%) pour filtrer les lignes dans une cellule
#define GRID_LINE_SIZE_THRESHOLD 0.85

// --- Constantes Générales ---
#define OUTPUT_DIR "output"
#define PI 3.14159265
#define TARGET_SIZE 28 

// --- STRUCTURES DE DONNÉES ---

// Représente un bloc 1D (ex: une ligne, une lettre) trouvé dans un profil
typedef struct {
    int start;
    int end;
} Block;

// Conteneur pour les données de l'image principale
typedef struct {
    GdkPixbuf *pixbuf;
    guchar *pixels;
    int width;
    int height;
    int rowstride;
    int n_channels;
} ImageData;

// Coordonnée 2D pour la pile du flood-fill
typedef struct { int x, y; } Point;
// Taille max de la pile pour le flood-fill (évite récursion infinie)
#define STACK_SIZE 10000 

// --- DÉCLARATIONS DE PROTOTYPES ---
void save_rect_as_28x28(ImageData *data, GdkRectangle rect, const char *path);
gint compare_integers(gconstpointer a, gconstpointer b);
int get_profile_max(int *profile, int profile_size);
int is_foreground(ImageData *data, int x, int y, int threshold);
GdkRectangle flood_fill(ImageData *data, GdkRectangle line_rect, int x, int y, 
                        gboolean **visited, int threshold);
int get_adaptive_threshold(ImageData *data, GdkRectangle rect);
void split_wide_blob(ImageData *data, GdkRectangle blob_rect, int word_num, 
                     int *letter_num_ptr, int threshold);
void process_list_with_projection(ImageData *data, GdkRectangle zone, int list_threshold);
void sobel_at(ImageData *data, int x, int y, int *gx, int *gy);
int create_edge_profiles(ImageData *data, GdkRectangle zone, int **h_profile, int **v_profile);
GList* get_peak_centers(GList *blocks);
GList* group_lines(GList *centers, int min_dist);
void find_and_save_grid_letter(ImageData *data, GdkRectangle cell_rect, const char *filename);
void process_grid_with_hough(ImageData *data, GdkRectangle zone);


// --- FONCTIONS UTILITAIRES DE BASE ---

/**
 * Récupère la valeur de gris (0-255) d'un pixel (x, y).
 * Gère les dépassements de limites (retourne 255 = blanc).
 */
guchar get_pixel_gray(ImageData *data, int x, int y) {
    if (x < 0 || x >= data->width || y < 0 || y >= data->height) {
        return 255; 
    }
    guchar *p = data->pixels + y * data->rowstride + x * data->n_channels;
    return p[0]; // En niveaux de gris, R=G=B
}

/**
 * Sauvegarde une portion d'image (rect) dans un fichier PNG de 28x28.
 * Crée un canvas blanc de 28x28.
 * Redimensionne (en gardant les proportions) le 'rect' source s'il est trop grand.
 * Colle la lettre redimensionnée au centre du canvas.
 */
void save_rect_as_28x28(ImageData *data, GdkRectangle rect, const char *path) {
    if (rect.width <= 0 || rect.height <= 0) return;

    // 1. Extraire la lettre de l'image originale
    GdkPixbuf *src_pixbuf = gdk_pixbuf_new_subpixbuf(
        data->pixbuf, rect.x, rect.y, rect.width, rect.height);
    if (!src_pixbuf) return;

    // 2. Créer le canvas de destination (28x28, blanc)
    GdkPixbuf *dest_pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, TARGET_SIZE, TARGET_SIZE);
    gdk_pixbuf_fill(dest_pixbuf, 0xFFFFFFFF); // Remplir en blanc

    // 3. Redimensionner si la source est trop grande, en gardant les proportions
    int scaled_w = rect.width;
    int scaled_h = rect.height;
    GdkPixbuf *scaled_src = NULL;

    if (scaled_w > TARGET_SIZE || scaled_h > TARGET_SIZE) {
        double ratio = (double)scaled_w / (double)scaled_h;
        if (ratio > 1) { // Plus large
            scaled_w = TARGET_SIZE;
            scaled_h = (int)(TARGET_SIZE / ratio);
        } else { // Plus haut
            scaled_h = TARGET_SIZE;
            scaled_w = (int)(TARGET_SIZE * ratio);
        }
        scaled_src = gdk_pixbuf_scale_simple(src_pixbuf, scaled_w, scaled_h, GDK_INTERP_BILINEAR);
        g_object_unref(src_pixbuf); // Libérer l'original
    } else {
        scaled_src = src_pixbuf; // Pas besoin de redimensionner
    }
    
    // 4. Calculer la position centrée
    int dest_x = (TARGET_SIZE - scaled_w) / 2;
    int dest_y = (TARGET_SIZE - scaled_h) / 2;

    // 5. Coller la lettre sur le canvas blanc
    gdk_pixbuf_composite(
        scaled_src, dest_pixbuf,
        dest_x, dest_y, scaled_w, scaled_h,
        dest_x, dest_y, 1.0, 1.0,
        GDK_INTERP_BILINEAR, 255
    );

    // 6. Sauvegarder
    GError *error = NULL;
    if (!gdk_pixbuf_save(dest_pixbuf, path, "png", &error, NULL)) {
        g_printerr("Erreur sauvegarde %s: %s\n", path, error->message);
        g_error_free(error);
    }

    // 7. Nettoyer
    g_object_unref(scaled_src);
    g_object_unref(dest_pixbuf);
}


/**
 * Analyse un histogramme 1D (profil) et retourne une GList de 'Block'.
 * Chaque 'Block' représente un segment où le profil est > 'threshold'.
 */
GList* find_blocks(int *profile, int profile_size, int threshold) {
    GList *blocks = NULL;
    gboolean in_block = FALSE;
    int block_start = 0;

    for (int i = 0; i < profile_size; i++) {
        if (!in_block && profile[i] > threshold) {
            in_block = TRUE;
            block_start = i;
        } else if (in_block && profile[i] <= threshold) {
            in_block = FALSE;
            Block *b = g_malloc(sizeof(Block));
            b->start = block_start;
            b->end = i;
            blocks = g_list_append(blocks, b);
        }
    }
    if (in_block) { // Gérer le bloc final
        Block *b = g_malloc(sizeof(Block));
        b->start = block_start;
        b->end = profile_size;
        blocks = g_list_append(blocks, b);
    }
    return blocks;
}

// --- SECTION : LOGIQUE DE SEUIL ADAPTATIF (OTSU) ---

/**
 * Calcule le seuil de binarisation optimal pour une région donnée (rect).
 * Utilise la méthode d'Otsu, qui maximise la variance inter-classe.
 * L'histogramme est construit, puis le seuil 't' qui sépare le mieux
 * les pixels de fond et de premier plan est trouvé.
 */
int get_adaptive_threshold(ImageData *data, GdkRectangle rect) {
    long histogram[256] = {0};
    long total_pixels = 0;

    // 1. Construire l'histogramme de la région
    for (int y = rect.y; y < rect.y + rect.height; y++) {
        for (int x = rect.x; x < rect.x + rect.width; x++) {
            guchar gray = get_pixel_gray(data, x, y);
            histogram[gray]++;
            total_pixels++;
        }
    }
    if (total_pixels == 0) return 128; // Région vide

    long sum = 0;
    for (int t = 0; t < 256; t++) {
        sum += t * histogram[t];
    }

    long sumB = 0;
    long wB = 0;
    long wF = 0;
    double max_variance = 0.0;
    int best_threshold = 0;

    // 2. Trouver le seuil qui maximise la variance inter-classe
    for (int t = 0; t < 256; t++) {
        wB += histogram[t]; // Poids du Fond
        if (wB == 0) continue;

        wF = total_pixels - wB; // Poids du Premier Plan
        if (wF == 0) break;

        sumB += (long)t * histogram[t];

        double mB = (double)sumB / wB; // Moyenne Fond
        double mF = (double)(sum - sumB) / wF; // Moyenne Premier Plan

        double variance = (double)wB * (double)wF * (mB - mF) * (mB - mF);

        if (variance > max_variance) {
            max_variance = variance;
            best_threshold = t;
        }
    }
    
    // Si Otsu échoue (histogramme non-bimodal), retourner un seuil "sûr"
    if (best_threshold == 0 || best_threshold == 255) {
        return 200; 
    }

    return best_threshold;
}


// --- SECTION : LOGIQUE DE TRAITEMENT DE LA LISTE DE MOTS (CCA) ---

/**
 * Fonction utilitaire simple.
 * Renvoie 1 (vrai) si le pixel (x, y) est plus sombre que le 'threshold'.
 */
int is_foreground(ImageData *data, int x, int y, int threshold) {
    return (get_pixel_gray(data, x, y) < threshold);
}

/**
 * Fonction utilitaire.
 * Trouve la valeur maximale dans un profil (histogramme) 1D.
 */
int get_profile_max(int *profile, int profile_size) {
    int max = 0;
    for (int i = 0; i < profile_size; i++) {
        if (profile[i] > max) {
            max = profile[i];
        }
    }
    return max;
}


/**
 * Algorithme de Flood Fill (basé sur pile) pour trouver un "blob".
 * Démarre au point (x, y) (relatif à line_rect) et explore tous les
 * pixels connectés (4-connectivité) en utilisant le 'threshold' donné.
 * Marque les pixels dans 'visited' pour éviter les re-scans.
 * Retourne le GdkRectangle (coordonnées absolues) englobant le blob.
 */
GdkRectangle flood_fill(ImageData *data, GdkRectangle line_rect, int x, int y, 
                        gboolean **visited, int threshold) {
    Point stack[STACK_SIZE];
    int stack_top = 0;

    stack[stack_top++] = (Point){x, y};
    visited[y][x] = TRUE;

    int min_x = x, max_x = x, min_y = y, max_y = y;

    while (stack_top > 0) {
        Point p = stack[--stack_top];

        // Mettre à jour le rectangle englobant
        if (p.x < min_x) min_x = p.x;
        if (p.x > max_x) max_x = p.x;
        if (p.y < min_y) min_y = p.y;
        if (p.y > max_y) max_y = p.y;

        int dx[] = {0, 0, 1, -1};
        int dy[] = {1, -1, 0, 0};

        for (int i = 0; i < 4; i++) {
            int nx = p.x + dx[i];
            int ny = p.y + dy[i];

            // Vérifier les limites de la ligne (coordonnées relatives)
            if (nx >= 0 && nx < line_rect.width && ny >= 0 && ny < line_rect.height) {
                // Si c'est du texte et pas encore visité
                if (!visited[ny][nx] && is_foreground(data, line_rect.x + nx, line_rect.y + ny, threshold)) {
                    visited[ny][nx] = TRUE;
                    if (stack_top < STACK_SIZE) {
                        stack[stack_top++] = (Point){nx, ny};
                    }
                }
            }
        }
    }
    // Retourne le rectangle avec les coordonnées absolues de l'image
    return (GdkRectangle){line_rect.x + min_x, line_rect.y + min_y, max_x - min_x + 1, max_y - min_y + 1};
}

/**
 * Appelé quand un blob est "trop large" (ex: "PA", "AT" collés).
 * Effectue une projection verticale *à l'intérieur* de ce blob pour
 * trouver le "creux" (vallée) qui sépare les lettres.
 * Sauvegarde les sous-lettres trouvées.
 */
void split_wide_blob(ImageData *data, GdkRectangle blob_rect, int word_num, 
                     int *letter_num_ptr, int threshold) {
    
    // 1. Projection verticale sur le blob
    int *v_profile = g_new0(int, blob_rect.width);
    for (int x = 0; x < blob_rect.width; x++) {
        for (int y = 0; y < blob_rect.height; y++) {
            if (is_foreground(data, blob_rect.x + x, blob_rect.y + y, threshold)) {
                v_profile[x]++;
            }
        }
    }

    // 2. Trouver le seuil de "creux"
    int max_peak = get_profile_max(v_profile, blob_rect.width);
    int split_threshold = (int)(max_peak * SPLIT_VALLEY_FACTOR); 
    if (split_threshold < LIST_HORIZ_NOISE_THRESHOLD) {
        split_threshold = LIST_HORIZ_NOISE_THRESHOLD;
    }

    // 3. Trouver les sous-lettres (les blocs séparés par le creux)
    GList *sub_letters = find_blocks(v_profile, blob_rect.width, split_threshold);

    // 4. Sauvegarder chaque sous-lettre
    for (GList *l = sub_letters; l != NULL; l = l->next) {
        Block *b = (Block *)l->data;
        GdkRectangle letter_rect = {
            blob_rect.x + b->start, blob_rect.y,
            b->end - b->start, blob_rect.height
        };
        
        char filename[256];
        snprintf(filename, sizeof(filename), "%s/word%dletter%d.png",
                 OUTPUT_DIR, word_num, (*letter_num_ptr)++);
        
        if (letter_rect.width > 1 && letter_rect.height > 1) {
            save_rect_as_28x28(data, letter_rect, filename);
        }
    }

    g_list_free_full(sub_letters, g_free);
    g_free(v_profile);
}


/**
 * Traite la 'zone' de la liste de mots en utilisant la logique CCA.
 * Reçoit le 'list_threshold' pré-calculé (par Otsu) depuis 'main'.
 */
void process_list_with_projection(ImageData *data, GdkRectangle zone, int list_threshold) {
    g_print("Traitement de la LISTE DE MOTS [x:%d, y:%d, w:%d, h:%d] (Seuil=%d)\n",
            zone.x, zone.y, zone.width, zone.height, list_threshold);

    // 1. Projection Horizontale pour trouver les lignes de mots
    int *horizontal_profile = g_new0(int, zone.height);
    for (int y = 0; y < zone.height; y++) {
        for (int x = 0; x < zone.width; x++) {
            if (is_foreground(data, zone.x + x, zone.y + y, list_threshold)) {
                horizontal_profile[y]++;
            }
        }
    }
    GList *lines = find_blocks(horizontal_profile, zone.height, LIST_HORIZ_NOISE_THRESHOLD);
    g_print("... trouvé %d mots (lignes).\n", g_list_length(lines));

    int word_num = 1;
    for (GList *l = lines; l != NULL; l = l->next, word_num++) {
        Block *line_block = (Block *)l->data;
        GdkRectangle line_rect = {
            zone.x, zone.y + line_block->start,
            zone.width, line_block->end - line_block->start
        };

        if (line_rect.height == 0 || line_rect.width == 0) continue;

        // 2. Préparer le tableau 'visited' pour le flood-fill
        gboolean **visited = g_new0(gboolean*, line_rect.height);
        for(int i = 0; i < line_rect.height; i++) {
            visited[i] = g_new0(gboolean, line_rect.width);
        }
        int letter_num = 1;

        // 3. Scanner la ligne pixel par pixel
        for (int y = 0; y < line_rect.height; y++) {
            for (int x = 0; x < line_rect.width; x++) {

                // 4. Si on trouve un pixel de lettre non visité...
                if (!visited[y][x] && is_foreground(data, line_rect.x + x, line_rect.y + y, list_threshold)) {
                    
                    // 5. ...lancer le flood-fill pour trouver le blob entier
                    GdkRectangle blob_rect = flood_fill(data, line_rect, x, y, visited, list_threshold);

                    if (blob_rect.width <= 1 || blob_rect.height <= 1) continue;
                    
                    // 6. Analyser la forme (ratio) du blob
                    double aspect_ratio = (double)blob_rect.width / (double)blob_rect.height;

                    if (aspect_ratio > LETTER_ASPECT_RATIO_THRESHOLD) {
                        // 7a. Blob "Large" (ex: "PA") -> Appeler la fonction de coupe
                        split_wide_blob(data, blob_rect, word_num, &letter_num, list_threshold);
                    } else {
                        // 7b. Blob "Normal" (ex: "K") -> Sauvegarder directement
                        char filename[256];
                        snprintf(filename, sizeof(filename), "%s/word%dletter%d.png",
                                 OUTPUT_DIR, word_num, letter_num++);
                        save_rect_as_28x28(data, blob_rect, filename);
                    }
                }
            }
        }

        // 8. Nettoyer le tableau visited pour cette ligne
        for(int i = 0; i < line_rect.height; i++) {
            g_free(visited[i]);
        }
        g_free(visited);
    }

    g_list_free_full(lines, g_free);
    g_free(horizontal_profile);
}


// --- SECTION : LOGIQUE DE TRAITEMENT DE LA GRILLE (HOUGH) ---

/**
 * Calcule les gradients Gx et Gy à (x, y) en utilisant les noyaux de Sobel 3x3.
 */
void sobel_at(ImageData *data, int x, int y, int *gx, int *gy) {
    const int Gx[3][3] = {{-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1}};
    const int Gy[3][3] = {{-1, -2, -1}, {0, 0, 0}, {1, 2, 1}};

    *gx = 0;
    *gy = 0;

    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            int val = get_pixel_gray(data, x + j, y + i);
            *gx += val * Gx[i + 1][j + 1];
            *gy += val * Gy[i + 1][j + 1];
        }
    }
}

/**
 * Crée des "profils de contours" (histogrammes) horizontaux et verticaux.
 * C'est une Transformée de Hough simplifiée (ne cherche que 0/90 degrés).
 * 1. Calcule le gradient (Sobel) à chaque pixel.
 * 2. Si la magnitude > SOBEL_EDGE_THRESHOLD:
 * 3. Calcule l'angle du gradient.
 * 4. Si l'angle est (presque) vertical, incrémente v_profile[x].
 * 5. Si l'angle est (presque) horizontal, incrémente h_profile[y].
 * Retourne le nombre total de pixels de contour trouvés (pour 'main').
 */
int create_edge_profiles(ImageData *data, GdkRectangle zone, int **h_profile, int **v_profile) {
    *h_profile = g_new0(int, zone.height);
    *v_profile = g_new0(int, zone.width);
    int total_edges = 0;

    for (int y = 0; y < zone.height; y++) {
        for (int x = 0; x < zone.width; x++) {
            int gx, gy;
            sobel_at(data, zone.x + x, zone.y + y, &gx, &gy);

            double magnitude = sqrt(gx * gx + gy * gy);

            if (magnitude > SOBEL_EDGE_THRESHOLD) {
                total_edges++;
                double angle = atan2(gy, gx) * 180.0 / PI; 

                // Angle proche de +/- 90 degrés (contour horizontal)
                if ( (angle > (90.0 - HOUGH_ANGLE_TOLERANCE) && angle < (90.0 + HOUGH_ANGLE_TOLERANCE)) ||
                     (angle > (-90.0 - HOUGH_ANGLE_TOLERANCE) && angle < (-90.0 + HOUGH_ANGLE_TOLERANCE)) )
                {
                    (*h_profile)[y]++;
                }
                
                // Angle proche de 0 ou +/- 180 degrés (contour vertical)
                if ( (angle > (0.0 - HOUGH_ANGLE_TOLERANCE) && angle < (0.0 + HOUGH_ANGLE_TOLERANCE)) ||
                     (angle > (180.0 - HOUGH_ANGLE_TOLERANCE) || angle < (-180.0 + HOUGH_ANGLE_TOLERANCE)) )
                {
                    (*v_profile)[x]++;
                }
            }
        }
    }
    return total_edges;
}

/**
 * Fonction utilitaire.
 * Prend une GList de 'Block's (pics) et retourne une GList de
 * leurs positions centrales (gint*).
 */
GList* get_peak_centers(GList *blocks) {
    GList *centers = NULL;
    for (GList *l = blocks; l != NULL; l = l->next) {
        Block *b = (Block *)l->data;
        int center = b->start + (b->end - b->start) / 2;
        centers = g_list_append(centers, GINT_TO_POINTER(center));
    }
    return centers;
}

/**
 * Fonction utilitaire pour g_list_sort. Compare deux gint*
 */
gint compare_integers(gconstpointer a, gconstpointer b) {
    gint int_a = GPOINTER_TO_INT(a);
    gint int_b = GPOINTER_TO_INT(b);
    return int_a - int_b;
}

/**
 * Fusionne les détections de lignes multiples (ex: lignes épaisses).
 * Prend une GList de positions (centers), les trie, puis fusionne
 * (par moyenne) tous les 'centers' qui sont plus proches que 'min_dist'.
 * Retourne une GList de positions de lignes fusionnées.
 */
GList* group_lines(GList *centers, int min_dist) {
    if (!centers) return NULL;
    GList *grouped_lines = NULL;
    centers = g_list_sort(centers, (GCompareFunc)compare_integers);

    int group_start = GPOINTER_TO_INT(centers->data);
    int group_count = 1;

    for (GList *l = centers->next; l != NULL; l = l->next) {
        int pos = GPOINTER_TO_INT(l->data);
        if (pos - group_start < min_dist) {
            // Dans le même groupe -> calculer la moyenne
            group_start = (group_start * group_count + pos) / (group_count + 1);
            group_count++;
        } else {
            // Fin du groupe, enregistrer la moyenne
            grouped_lines = g_list_append(grouped_lines, GINT_TO_POINTER(group_start));
            // Commencer un nouveau groupe
            group_start = pos;
            group_count = 1;
        }
    }
    // Enregistrer le dernier groupe
    grouped_lines = g_list_append(grouped_lines, GINT_TO_POINTER(group_start));
    
    g_list_free(centers);
    return grouped_lines;
}

/**
 * Traite une cellule unique de la grille.
 * 1. Calcule un seuil local (Min/Max) pour cette cellule (évite l'affinage).
 * 2. Prépare un tableau 'visited'.
 * 3. Scanne la cellule *entière* (cell_rect) pour trouver tous les blobs.
 * 4. Filtre les blobs : ignore ceux qui sont trop larges/hauts
 * (considérés comme des lignes de grille) en utilisant GRID_LINE_SIZE_THRESHOLD.
 * 5. Fusionne tous les blobs restants (les parties de lettre, ex: 'H')
 * en un seul 'master_rect'.
 * 6. Sauvegarde le 'master_rect' final en 28x28.
 */
void find_and_save_grid_letter(ImageData *data, GdkRectangle cell_rect, const char *filename) {

    if (cell_rect.width <= 0 || cell_rect.height <= 0) return;

    // 1. Calculer le seuil local (Min/Max)
    guchar min_gray = 255;
    guchar max_gray = 0;
    gboolean has_pixels = FALSE;

    for (int y = 0; y < cell_rect.height; y++) {
        for (int x = 0; x < cell_rect.width; x++) {
            guchar gray = get_pixel_gray(data, cell_rect.x + x, cell_rect.y + y);
            if (gray < min_gray) min_gray = gray;
            if (gray > max_gray) max_gray = gray;
            has_pixels = TRUE;
        }
    }
    if (!has_pixels || min_gray >= max_gray) return;
    int cell_threshold = ( (int)min_gray + (int)max_gray + 1 ) / 2;

    // 2. Préparer 'visited'
    gboolean **visited = g_new0(gboolean*, cell_rect.height);
    for(int i = 0; i < cell_rect.height; i++) {
        visited[i] = g_new0(gboolean, cell_rect.width);
    }

    gboolean letter_blob_found = FALSE;
    GdkRectangle master_rect;
    GList *letter_blobs = NULL; // Stocke les parties de lettre

    // 3. Scanner la cellule (sans padding)
    for (int y = 0; y < cell_rect.height; y++) {
        for (int x = 0; x < cell_rect.width; x++) {

            if (!visited[y][x] && is_foreground(data, cell_rect.x + x, cell_rect.y + y, cell_threshold)) {

                // 4. Trouver un blob
                GdkRectangle blob_rect = flood_fill(data, cell_rect, x, y, visited, cell_threshold);

                if (blob_rect.width <= 1 && blob_rect.height <= 1) continue;

                // 5. Filtrer les lignes de la grille
                gboolean is_line = FALSE;
                if ( (double)blob_rect.width > (double)cell_rect.width * GRID_LINE_SIZE_THRESHOLD ) {
                    is_line = TRUE; // Ligne horizontale
                }
                if ( (double)blob_rect.height > (double)cell_rect.height * GRID_LINE_SIZE_THRESHOLD ) {
                    is_line = TRUE; // Ligne verticale
                }

                // 6. Si ce n'est PAS une ligne, on garde
                if (!is_line) {
                    GdkRectangle *rect_copy = g_new(GdkRectangle, 1);
                    *rect_copy = blob_rect;
                    letter_blobs = g_list_append(letter_blobs, rect_copy);
                }
            }
        }
    }

    // 7. Fusionner tous les blobs de lettre (ex: parties de 'H')
    if (letter_blobs != NULL) {
        master_rect = *( (GdkRectangle*)letter_blobs->data );
        letter_blob_found = TRUE;
        
        for (GList *l = letter_blobs->next; l != NULL; l = l->next) {
            gdk_rectangle_union(&master_rect, (GdkRectangle*)l->data, &master_rect);
        }
    }

    // 8. Sauvegarder
    if (letter_blob_found) {
        save_rect_as_28x28(data, master_rect, filename);
    }

    // 9. Nettoyer
    g_list_free_full(letter_blobs, g_free);
    for(int i = 0; i < cell_rect.height; i++) {
        g_free(visited[i]);
    }
    g_free(visited);
}


/**
 * Traite la 'zone' de la grille en utilisant la Transformée de Hough.
 * 1. Crée les profils de contours (Hough) pour la zone.
 * 2. Trouve les "pics" (lignes) dans ces profils.
 * 3. Fusionne les lignes trop proches (group_lines).
 * 4. Itère sur chaque cellule (r, c) définie par les lignes.
 * 5. Appelle find_and_save_grid_letter pour la cellule (sans padding).
 */
void process_grid_with_hough(ImageData *data, GdkRectangle zone) {
    g_print("Traitement de la GRILLE [x:%d, y:%d, w:%d, h:%d]\n",
            zone.x, zone.y, zone.width, zone.height);

    int *h_profile, *v_profile;
    create_edge_profiles(data, zone, &h_profile, &v_profile);

    // 2. Trouver les pics (blocs)
    GList *h_line_blocks = find_blocks(h_profile, zone.height, HOUGH_PEAK_THRESHOLD);
    GList *v_line_blocks = find_blocks(v_profile, zone.width, HOUGH_PEAK_THRESHOLD);

    // 3. Trouver les centres et fusionner
    GList *h_centers = get_peak_centers(h_line_blocks);
    GList *v_centers = get_peak_centers(v_line_blocks);
    GList *h_lines = group_lines(h_centers, LINE_GROUPING_DISTANCE);
    GList *v_lines = group_lines(v_centers, LINE_GROUPING_DISTANCE);

    int num_rows = g_list_length(h_lines) - 1;
    int num_cols = g_list_length(v_lines) - 1;
    g_print("... trouvé %d lignes horizontales et %d lignes verticales.\n", num_rows + 1, num_cols + 1);
    g_print("... grille détectée: %d Lignes x %d Colonnes.\n", num_rows, num_cols);
    if (num_rows <= 0 || num_cols <= 0) {
        g_printerr("Erreur: Grille non détectée. Ajustez les seuils SOBEL/HOUGH.\n");
        return;
    }

    // 4. Itérer sur les cellules
    for (int r = 0; r < num_rows; r++) {
        for (int c = 0; c < num_cols; c++) {
            // Définir la cellule par les lignes fusionnées
            int y1 = GPOINTER_TO_INT(g_list_nth_data(h_lines, r));
            int y2 = GPOINTER_TO_INT(g_list_nth_data(h_lines, r + 1));
            int x1 = GPOINTER_TO_INT(g_list_nth_data(v_lines, c));
            int x2 = GPOINTER_TO_INT(g_list_nth_data(v_lines, c + 1));

            // C'est la cellule *entière*, incluant les lignes de grille
            GdkRectangle cell_rect = {
                zone.x + x1, zone.y + y1,
                x2 - x1, y2 - y1
            };
            
            char filename[256];
            snprintf(filename, sizeof(filename), "%s/row%dcol%d.png",
                     OUTPUT_DIR, r + 1, c + 1);
            
            // 5. Traiter la cellule (sans padding)
            find_and_save_grid_letter(data, cell_rect, filename);
        }
    }

    // 6. Nettoyer
    g_list_free(h_lines);
    g_list_free(v_lines);
    g_list_free_full(h_line_blocks, g_free);
    g_list_free_full(v_line_blocks, g_free);
    g_free(h_profile);
    g_free(v_profile);
}


// --- PROGRAMME PRINCIPAL ---
int cut_grid(int argc, char *argv[])
{
    // 1. Gestion des arguments
    if (argc != 2) {
        g_printerr("Usage: %s <image.bmp>\n", argv[0]);
        return 1;
    }
    
    char *filename = argv[1];
    GError *error = NULL;

    // 2. Gestion du dossier de sortie (supprimer s'il existe)
    struct stat st = {0};
    if (stat(OUTPUT_DIR, &st) != -1) {
        g_print("Suppression de l'ancien dossier '%s'...\n", OUTPUT_DIR);
        int ret = system("rm -r " OUTPUT_DIR);
        if (ret != 0) {
            g_printerr("Erreur: Impossible de supprimer le dossier %s.\n", OUTPUT_DIR);
            return 1;
        }
    }
    if (mkdir(OUTPUT_DIR, 0755) != 0) {
        g_printerr("Erreur: Impossible de créer le dossier %s\n", OUTPUT_DIR);
        return 1;
    }

    // 3. Chargement de l'image
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(filename, &error);
    if (error) {
        g_printerr("Erreur chargement %s: %s\n", filename, error->message);
        g_error_free(error);
        return 1;
    }

    // 4. Initialisation de la structure de données
    ImageData data;
    data.pixbuf = pixbuf;
    data.pixels = gdk_pixbuf_get_pixels(pixbuf);
    data.width = gdk_pixbuf_get_width(pixbuf);
    data.height = gdk_pixbuf_get_height(pixbuf);
    data.rowstride = gdk_pixbuf_get_rowstride(pixbuf);
    data.n_channels = gdk_pixbuf_get_n_channels(pixbuf);

    g_print("Image chargée: %d x %d (canaux: %d)\n",
            data.width, data.height, data.n_channels);

    // --- ÉTAPE 1 : Séparer Liste et Grille ---
    // Projection verticale sur l'image entière pour trouver le plus grand espace
    int *v_profile = g_new0(int, data.width);
    for (int x = 0; x < data.width; x++) {
        for (int y = 0; y < data.height; y++) {
            if (get_pixel_gray(&data, x, y) < 200) { // Seuil "sûr"
                v_profile[x]++;
            }
        }
    }
    
    GList *v_blocks = find_blocks(v_profile, data.width, MAIN_SEPARATOR_THRESHOLD);
    int split_x = 0;
    int max_gap = 0;

    if (g_list_length(v_blocks) < 2) {
         g_printerr("Erreur: N'a pas pu trouver 2 blocs (liste et grille).\n");
         return 1;
    }

    for (GList *l = v_blocks; l != NULL && l->next != NULL; l = l->next) {
        Block *b1 = (Block *)l->data;
        Block *b2 = (Block *)l->next->data;
        int gap = b2->start - b1->end;
        if (gap > max_gap) {
            max_gap = gap;
            split_x = b1->end + (gap / 2);
        }
    }
    g_list_free_full(v_blocks, g_free);
    g_free(v_profile);

    if (split_x == 0) {
        g_printerr("Erreur: Pas d'espace trouvé entre les blocs.\n");
        g_object_unref(pixbuf);
        return 1;
    }
    g_print("Séparation détectée à x = %d\n", split_x);

    GdkRectangle zone_A = {0, 0, split_x, data.height}; // Zone Gauche
    GdkRectangle zone_B = {split_x, 0, data.width - split_x, data.height}; // Zone Droite

    // --- ÉTAPE 2 : Identifier quelle zone est la Grille ---
    // (Celle avec le plus de contours = lignes)
    int *hA, *vA, *hB, *vB;
    int edges_A = create_edge_profiles(&data, zone_A, &hA, &vA);
    int edges_B = create_edge_profiles(&data, zone_B, &hB, &vB);
    g_free(hA); g_free(vA); g_free(hB); g_free(vB);

    g_print("Détection: Zone A (gauche) a %d contours. Zone B (droite) a %d contours.\n", edges_A, edges_B);

    GdkRectangle grid_zone, list_zone;
    if (edges_A > edges_B) {
        g_print(" -> Zone A = Grille, Zone B = Liste\n");
        grid_zone = zone_A;
        list_zone = zone_B;
    } else {
        g_print(" -> Zone A = Liste, Zone B = Grille\n");
        grid_zone = zone_B;
        list_zone = zone_A;
    }
    
    // --- ÉTAPE 2.5 : Calculer le seuil pour la LISTE (Otsu) ---
    // (La grille utilise un seuil Min/Max local pour chaque cellule)
    g_print("Calcul du seuil adaptatif pour la liste...\n");
    int list_threshold = get_adaptive_threshold(&data, list_zone);
    g_print("Seuil Liste = %d\n", list_threshold);


    // --- ÉTAPE 3 & 4 : Traiter les zones ---
    process_grid_with_hough(&data, grid_zone);
    process_list_with_projection(&data, list_zone, list_threshold);

    g_print("Traitement terminé.\n");
    g_object_unref(pixbuf);
    return 0;
}
