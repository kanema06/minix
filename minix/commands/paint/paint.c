#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#define FILAS 24
#define COLS 80

int main() {
    int fd;
    unsigned char paquete[3];
    char pantalla[FILAS][COLS];
    
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

    while (1) {
        if (read(fd, paquete, 3) == 3) {
            
            int click_izq = paquete[0] & 0x01;
            int click_der = (paquete[0] & 0x02) >> 1;
            
            int dx = (signed char)paquete[1];
            int dy = (signed char)paquete[2];

            printf("\033[%d;%dH%c", y + 1, x + 1, pantalla[y][x]);

            if (click_izq) {
                pantalla[y][x] = '#'; 
                printf("\033[%d;%dH#", y + 1, x + 1); 
            } else if (click_der) {
                pantalla[y][x] = ' '; 
                printf("\033[%d;%dH ", y + 1, x + 1); 
            }

            int nuevo_x = x + (dx / 4);
            int nuevo_y = y - (dy / 4); 

            if (nuevo_x < 0) nuevo_x = 0;
            if (nuevo_x >= COLS) nuevo_x = COLS - 1;
            if (nuevo_y < 0) nuevo_y = 0;
            if (nuevo_y >= FILAS) nuevo_y = FILAS - 1;

            x = nuevo_x;
            y = nuevo_y;

            
            printf("\033[%d;%dH+", y + 1, x + 1);
            fflush(stdout);
        }
    }

    close(fd);
    return 0;
}