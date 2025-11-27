#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"
#include "png_to_bmp.h"
#include <stdio.h>
#include <dirent.h>
#include <string.h>

void convert_folder(const char* folder) {
    DIR* dir = opendir(folder);
    if (!dir) {
        printf("❌ Impossible d'ouvrir : %s\n", folder);
        return;
    }

    struct dirent* entry;
    char input[256], output[256];

    while ((entry = readdir(dir)) != NULL) {

        // Ignorer les systèmes Mac et fichiers cachés
        if (entry->d_name[0] == '.') continue;

        // Si c'est un dossier, récursion
        if (entry->d_type == DT_DIR) {
            char subfolder[256];
            sprintf(subfolder, "%s/%s", folder, entry->d_name);
            convert_folder(subfolder);
            continue;
        }

        // Garder seulement les PNG
        if (!strstr(entry->d_name, ".png")) continue;

        sprintf(input, "%s/%s", folder, entry->d_name);

        // Nouveau nom BMP
        strcpy(output, input);
        output[strlen(output) - 4] = 0; // supprime ".png"
        strcat(output, ".bmp");

        int w, h, c;
        unsigned char* pixels = stbi_load(input, &w, &h, &c, 3);

        if (!pixels) {
            printf("⚠️ Erreur lecture PNG: %s\n", input);
            continue;
        }

        stbi_write_bmp(output, w, h, 3, pixels);
        stbi_image_free(pixels);

        // Supprimer le PNG original
        if (remove(input) == 0) {
            printf("✔ Converti et supprimé : %s → %s\n", input, output);
        } else {
            perror("⚠ Erreur suppression PNG");
        }
    }

    closedir(dir);
}

