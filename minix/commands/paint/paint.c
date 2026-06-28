#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <minix/input.h>

#define SCREEN_W 80
#define SCREEN_H 24

typedef struct {
    int x;
    int y;
} point_t;

static char grid[SCREEN_H][SCREEN_W];
static point_t pos;
static int left_down;
static int right_down;
static struct termios term_backup;

static void move_cursor(int row, int col)
{
    printf("\033[%d;%dH", row + 1, col + 1);
}

static int clamp(int value, int low, int high)
{
    if (value < low)
        return low;
    if (value > high)
        return high;
    return value;
}

static void enable_raw_mode(void)
{
    struct termios raw;

    tcgetattr(STDIN_FILENO, &term_backup);
    raw = term_backup;
    raw.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
}

static void on_exit_signal(int signo)
{
    (void)signo;
    printf("\033[?25h\033[2J\033[H");
    fflush(stdout);
    tcsetattr(STDIN_FILENO, TCSANOW, &term_backup);
    _exit(0);
}

static void handle_button_event(const struct input_event *ev)
{
    if (ev->code == INPUT_BUTTON_1)
        left_down = ev->value;
    else if (ev->code == INPUT_BUTTON_1 + 1)
        right_down = ev->value;
}

static void handle_motion_event(const struct input_event *ev)
{
    int new_x = pos.x;
    int new_y = pos.y;

    if (ev->code == INPUT_GD_X)
        new_x += ev->value;
    else if (ev->code == INPUT_GD_Y)
        new_y -= ev->value;

    new_x = clamp(new_x, 0, SCREEN_W - 1);
    new_y = clamp(new_y, 0, SCREEN_H - 1);

    if (new_x == pos.x && new_y == pos.y)
        return;

    /* repintar la celda que el cursor deja atras */
    move_cursor(pos.y, pos.x);
    putchar(grid[pos.y][pos.x]);

    pos.x = new_x;
    pos.y = new_y;

    if (left_down)
        grid[pos.y][pos.x] = '#';
    if (right_down)
        grid[pos.y][pos.x] = ' ';

    move_cursor(pos.y, pos.x);
    putchar('+');
    fflush(stdout);
}

int main(void)
{
    int mouse_fd;
    struct input_event ev;

    mouse_fd = open("/dev/mouse0", O_RDONLY);
    if (mouse_fd < 0) {
        perror("/dev/mouse0");
        return 1;
    }

    enable_raw_mode();
    signal(SIGINT, on_exit_signal);

    memset(grid, ' ', sizeof(grid));
    printf("\033[2J\033[?25l");

    pos.x = SCREEN_W / 2;
    pos.y = SCREEN_H / 2;
    move_cursor(pos.y, pos.x);
    putchar('+');
    fflush(stdout);

    while (read(mouse_fd, &ev, sizeof(ev)) == (ssize_t)sizeof(ev)) {
        if (ev.page == INPUT_PAGE_BUTTON)
            handle_button_event(&ev);
        else if (ev.page == INPUT_PAGE_GD)
            handle_motion_event(&ev);
    }

    close(mouse_fd);
    on_exit_signal(0);
    return 0;
}