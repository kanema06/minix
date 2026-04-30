#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

int total_archivos = 0;
int total_directorios = 0;
int es_rama_ultima[1024];

void inspeccionar(const char *ruta_actual, int nivel, int rama_es_ultima) {
    DIR *flujo_dir = opendir(ruta_actual);
    if (!flujo_dir) return;

    struct dirent *entrada;
    struct dirent *proxima_entrada;

    entrada = readdir(flujo_dir);
    while (entrada != NULL) {
        if (entrada->d_name[0] == '.') {
            entrada = readdir(flujo_dir);
            continue;
        }

        for (int i = 0; i < nivel; i++) {
            if (es_rama_ultima[i])
                printf("     ");
            else
                printf("|    ");
        }

        proxima_entrada = readdir(flujo_dir);
        if (proxima_entrada == NULL)
            printf("`-- %s\n", entrada->d_name);
        else
            printf("|-- %s\n", entrada->d_name);

        es_rama_ultima[nivel] = (proxima_entrada == NULL);

        char ruta_completa[1024];
        snprintf(ruta_completa, sizeof(ruta_completa), "%s/%s", ruta_actual, entrada->d_name);

        struct stat info_archivo;
        if (lstat(ruta_completa, &info_archivo) == 0) {
            if (S_ISDIR(info_archivo.st_mode)) {
                total_directorios++;
                inspeccionar(ruta_completa, nivel + 1, proxima_entrada == NULL);
            } else {
                total_archivos++;
            }
        } else {
            total_archivos++;
        }

        entrada = proxima_entrada;
    }

    closedir(flujo_dir);
}

void construir_arbol(const char *punto_partida) {
    total_archivos = 0;
    total_directorios = 0;

    printf("%s\n", punto_partida);
    inspeccionar(punto_partida, 0, 1);

    printf("\n%d directories, %d files\n", total_directorios + 1, total_archivos);
}

int main(int argc, char *argv[]) {
    const char *ruta = (argc > 1) ? argv[1] : ".";
    construir_arbol(ruta);
    return 0;
}