#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

int total_archivos = 0;
int total_directorios = 0;
int es_rama_ultima[1024];

void inspeccionar(const char *ruta_actual, int nivel, int rama_es_ultima) {
    DIR *flujo_dir;
    struct dirent *entrada;
    struct dirent *proxima_entrada;
    char ruta_completa[1024];
    struct stat info_archivo;
    int i;
    
    flujo_dir = opendir(ruta_actual);
    if (flujo_dir == NULL) return;

    entrada = readdir(flujo_dir);
    while (entrada != NULL) {
        if (entrada->d_name[0] == '.') {
            entrada = readdir(flujo_dir);
            continue;
        }

        for (i = 0; i < nivel; i++) {
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

        sprintf(ruta_completa, "%s/%s", ruta_actual, entrada->d_name);

        if (lstat(ruta_completa, &info_archivo) == 0) {
            if (info_archivo.st_mode & S_IFDIR) {
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