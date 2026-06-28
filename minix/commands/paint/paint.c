#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 25

#define CURSOR_CHAR 'O'
#define BRUSH_CHAR  '#'
#define ERASER_CHAR ' '

int main() {
    int fd = open("/dev/mouse0", O_RDONLY);
    if (fd < 0) {
        perror("Error: No se pudo abrir /dev/mouse0. Revisa tu driver");
        return 1;
    }

    struct termios old_tio, new_tio;
    tcgetattr(STDIN_FILENO, &old_tio);
    new_tio = old_tio;
    new_tio.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);

    printf("\033[2J");

    int x = SCREEN_WIDTH / 2;
    int y = SCREEN_HEIGHT / 2;

    unsigned char packet[3];
    
    while (1) {
        printf("\033[%d;%dH%c", y, x, CURSOR_CHAR);
        fflush(stdout);

        int bytes = read(fd, packet, 3);
        
        if (bytes == 3) {

            int left_click = packet[0] & 0x01;
            int right_click = packet[0] & 0x02;

            char dx = (char)packet[1]; 
            char dy = (char)packet[2]; 
            
            printf("\033[%d;%dH", y, x);
            if (left_click) {
                printf("%c", BRUSH_CHAR); 
            } else if (right_click) {
                printf("%c", ERASER_CHAR); 
            } else {
                printf(" "); 
            }

            x += dx;
            y -= dy;

            if (x < 1) x = 1;
            if (x > SCREEN_WIDTH) x = SCREEN_WIDTH;
            if (y < 1) y = 1;
            if (y > SCREEN_HEIGHT) y = SCREEN_HEIGHT;
        }
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &old_tio);
    close(fd);
    return 0;
}