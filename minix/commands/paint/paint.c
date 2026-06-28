#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>


#define MOUSE_DEV       "/dev/mouse0"

#define SCR_COLS        80    
#define SCR_ROWS        25     

#define CURSOR_CH       '+'   
#define BRUSH_CH        '*'   
#define ERASER_CH       ' '  

#define LEFT_BTN_MASK   0x01
#define RIGHT_BTN_MASK  0x02
#define XSIGN_MASK      0x10
#define YSIGN_MASK      0x20

typedef struct {
    int dx;     
    int dy;     
    int left;  
    int right;  
} mouse_event_t;


static int  mouse_fd = -1;              
static char screen[SCR_ROWS][SCR_COLS]; 
static int  cur_col, cur_row;             

static void move_to(int row, int col)
{
    char seq[16];
    int  len = snprintf(seq, sizeof(seq), "\033[%d;%dH", row + 1, col + 1);
    write(STDOUT_FILENO, seq, (size_t) len);
}

static void clear_screen(void)
{
    write(STDOUT_FILENO, "\033[2J", 4);
}

static void paint_cell(int row, int col, char ch)
{
    screen[row][col] = ch;
    move_to(row, col);
    write(STDOUT_FILENO, &ch, 1);
}

static void restore_cell(int row, int col)
{
    move_to(row, col);
    write(STDOUT_FILENO, &screen[row][col], 1);
}

static void draw_cursor_glyph(int row, int col)
{
    char ch = CURSOR_CH;
    move_to(row, col);
    write(STDOUT_FILENO, &ch, 1);
}

static void paint_segment(int x0, int y0, int x1, int y1, char ch)
{
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x1 >= x0) ? 1 : -1;
    int sy = (y1 >= y0) ? 1 : -1;
    int err = dx - dy;
    int x = x0, y = y0;

    for (;;) {
        paint_cell(y, x, ch);

        if (x == x1 && y == y1)
            break;

        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x += sx; }
        if (e2 <  dx) { err += dx; y += sy; }
    }
}

static int read_mouse_event(mouse_event_t *ev)
{
    static unsigned char buf[3];
    static int filled = 0;

 
    while (filled < 3) {
        ssize_t n = read(mouse_fd, buf + filled, 3 - filled);

        if (n == 0)
            return 0;         
        if (n < 0)
            return -1;         

        filled += (int) n;
    }
    filled = 0; 

    ev->left  = (buf[0] & LEFT_BTN_MASK)  ? 1 : 0;
    ev->right = (buf[0] & RIGHT_BTN_MASK) ? 1 : 0;

    int dx = buf[1];
    int dy = buf[2];
    if (buf[0] & XSIGN_MASK) dx -= 256;  /* completa el signo de X */
    if (buf[0] & YSIGN_MASK) dy -= 256;  /* completa el signo de Y */

    ev->dx =  dx;
    ev->dy = -dy;

    return 1;
}

static void cleanup_and_exit(int status)
{
    move_to(SCR_ROWS - 1, 0);
    write(STDOUT_FILENO, "\n", 1);

    if (mouse_fd >= 0)
        close(mouse_fd);

    _exit(status); 
}

static void on_sigint(int signo)
{
    (void) signo;
    cleanup_and_exit(0);
}

int main(void)
{
    memset(screen, ' ', sizeof(screen));

    mouse_fd = open(MOUSE_DEV, O_RDONLY);
    if (mouse_fd < 0) {
        perror("paint: no se pudo abrir " MOUSE_DEV);
        return 1;
    }

    signal(SIGINT, on_sigint);

    clear_screen();

    cur_col = SCR_COLS / 2;
    cur_row = SCR_ROWS / 2;
    draw_cursor_glyph(cur_row, cur_col);

    for (;;) {
        mouse_event_t ev;
        int r = read_mouse_event(&ev);

        if (r == 0)
            break;
        if (r < 0) {
            perror("paint: error leyendo " MOUSE_DEV);
            break;
        }

        int old_col = cur_col;
        int old_row = cur_row;

        cur_col += ev.dx;
        cur_row += ev.dy;

        if (cur_col < 0)            cur_col = 0;
        if (cur_col >= SCR_COLS)    cur_col = SCR_COLS - 1;
        if (cur_row < 0)            cur_row = 0;
        if (cur_row >= SCR_ROWS)    cur_row = SCR_ROWS - 1;

        if (ev.left) {
            paint_segment(old_col, old_row, cur_col, cur_row, BRUSH_CH);
        } else if (ev.right) {
            paint_segment(old_col, old_row, cur_col, cur_row, ERASER_CH);
        } else {
            restore_cell(old_row, old_col);
        }

        draw_cursor_glyph(cur_row, cur_col);
    }

    cleanup_and_exit(0);
    return 0; 
}