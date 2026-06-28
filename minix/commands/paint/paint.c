#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#define FILAS 24
#define COLS 80

int main() {
    int fd;
    unsigned char paquete[3];
    char pantalla[FILAS][COLS]; // Matriz bidimensional exigida
    
   for (int i = 0; i < FILAS; i++) {
        for (int j = 0; j < COLS; j++) {
            pantalla[i][j] = ' ';
        }
    }

    
    printf("\033[2J");
    
    int x = COLS / 2;
    int y = FILAS / 2;

    fd = open("/dev/mouse0", O_RDONLY);
    if (fd < 0) {
        perror("Error al abrir /dev/mouse0");
        return 1;
    }

    printf("\033[%d;%dH+", y + 1, x + 1);
    fflush(stdout);

    int acc_x = 0;
    int acc_y = 0;

    while (1) {
        unsigned char b;
        
        // INTERCEPCIÓN Y SINCRO: Leemos 1 solo byte para buscar la cabecera real
        if (read(fd, &b, 1) != 1) continue;
        
        if ((b & 0x08) == 0x08) {
            paquete[0] = b;
           if (read(fd, &paquete[1], 1) != 1) continue;
            if (read(fd, &paquete[2], 1) != 1) continue;
        } else {
            // Si el byte no tiene el bit de control, estamos desalineados; se descarta
            continue; 
        }
  int click_izq = paquete[0] & 0x01;
        int click_der = (paquete[0] & 0x02) >> 1;
        
         int dx = (signed char)paquete[1];
        int dy = (signed char)paquete[2];

       acc_x += dx;
        acc_y += dy;

        int mover_x = acc_x / 4;
        int mover_y = acc_y / 4;

        if (mover_x != 0 || mover_y != 0 || click_izq || click_der) {
           printf("\033[%d;%dH%c", y + 1, x + 1, pantalla[y][x]);

            if (click_izq) {
                pantalla[y][x] = '#'; // Guarda en la matriz
                printf("\033[%d;%dH#", y + 1, x + 1); // Dibuja en terminal
            } else if (click_der) {
                pantalla[y][x] = ' '; 
                printf("\033[%d;%dH ", y + 1, x + 1); 
            }

            x += mover_x;
            y -= mover_y; // Restar en Y hace que el cursor suba en la terminal

            acc_x %= 4;
            acc_y %= 4;

           if (x < 0) x = 0;
            if (x >= COLS) x = COLS - 1;
            if (y < 0) y = 0;
            if (y >= FILAS) y = FILAS - 1;

            // 6. Dibujar el CURSOR en la nueva posición
            printf("\033[%d;%dH+", y + 1, x + 1);
            fflush(stdout);
        }
    }

    close(fd);
    return 0;
}