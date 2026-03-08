#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
  #include <conio.h>
  #include <windows.h>
  #define CLEAR "cls"
  #define SLEEP(ms) Sleep(ms)
  int kbhit_custom() { return kbhit(); }
  char getch_custom() { return getch(); }
#else
  #include <unistd.h>
  #include <termios.h>
  #include <fcntl.h>
  #define CLEAR "clear"
  #define SLEEP(ms) usleep((ms) * 1000)

  static struct termios orig_termios;

  void enable_raw_mode() {
      struct termios raw = orig_termios;
      tcgetattr(STDIN_FILENO, &orig_termios);
      raw = orig_termios;
      raw.c_lflag &= ~(ICANON | ECHO);
      raw.c_cc[VMIN] = 0;
      raw.c_cc[VTIME] = 0;
      tcsetattr(STDIN_FILENO, TCSANOW, &raw);
  }

  void disable_raw_mode() {
      tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
  }

  int kbhit_custom() {
      char c;
      return read(STDIN_FILENO, &c, 1) == 1 ? (ungetc(c, stdin), 1) : 0;
  }

  char getch_custom() {
      char c = 0;
      read(STDIN_FILENO, &c, 1);
      return c;
  }
#endif

/* -------- 配置 -------- */
#define WIDTH   20
#define HEIGHT  15
#define MAX_LEN (WIDTH * HEIGHT)

/* -------- 数据结构 -------- */
typedef struct { int x, y; } Point;

typedef enum { UP, DOWN, LEFT, RIGHT } Dir;

Point  snake[MAX_LEN];
int    snake_len;
Dir    dir;
Point  food;
int    score;
int    game_over;

/* -------- 工具函数 -------- */
void place_food() {
    int valid;
    do {
        valid = 1;
        food.x = rand() % WIDTH;
        food.y = rand() % HEIGHT;
        for (int i = 0; i < snake_len; i++)
            if (snake[i].x == food.x && snake[i].y == food.y) { valid = 0; break; }
    } while (!valid);
}

void init_game() {
    snake_len    = 3;
    dir          = RIGHT;
    score        = 0;
    game_over    = 0;

    /* 蛇初始位置：中间偏左 */
    for (int i = 0; i < snake_len; i++) {
        snake[i].x = WIDTH / 2 - i;
        snake[i].y = HEIGHT / 2;
    }
    place_food();
}

/* -------- 渲染 -------- */
void draw() {
    /* 构建画面缓冲区，避免闪烁 */
    char buf[HEIGHT][WIDTH];
    memset(buf, ' ', sizeof(buf));

    /* 食物 */
    buf[food.y][food.x] = '@';

    /* 蛇身 */
    for (int i = 1; i < snake_len; i++)
        buf[snake[i].y][snake[i].x] = 'o';

    /* 蛇头 */
    buf[snake[0].y][snake[0].x] = 'O';

    /* 输出 */
    system(CLEAR);
    printf("╔");
    for (int i = 0; i < WIDTH; i++) printf("══");
    printf("╗\n");

    for (int r = 0; r < HEIGHT; r++) {
        printf("║");
        for (int c = 0; c < WIDTH; c++) printf(" %c", buf[r][c]);
        printf(" ║\n");
    }

    printf("╚");
    for (int i = 0; i < WIDTH; i++) printf("══");
    printf("╝\n");

    printf("  得分: %d    方向键/WASD 移动  Q 退出\n", score);
}

/* -------- 输入处理 -------- */
void handle_input() {
    if (!kbhit_custom()) return;

    char c = getch_custom();

    /* 方向键会产生两个字节：ESC + [ + A/B/C/D */
    if (c == '\033') {
        getch_custom(); /* '[' */
        c = getch_custom();
        switch (c) {
            case 'A': if (dir != DOWN)  dir = UP;    break;
            case 'B': if (dir != UP)    dir = DOWN;  break;
            case 'C': if (dir != LEFT)  dir = RIGHT; break;
            case 'D': if (dir != RIGHT) dir = LEFT;  break;
        }
        return;
    }

    /* WASD */
    switch (c) {
        case 'w': case 'W': if (dir != DOWN)  dir = UP;    break;
        case 's': case 'S': if (dir != UP)    dir = DOWN;  break;
        case 'd': case 'D': if (dir != LEFT)  dir = RIGHT; break;
        case 'a': case 'A': if (dir != RIGHT) dir = LEFT;  break;
        case 'q': case 'Q': game_over = 1; break;
    }
}

/* -------- 逻辑更新 -------- */
void update() {
    /* 计算新头部位置 */
    Point new_head = snake[0];
    switch (dir) {
        case UP:    new_head.y--; break;
        case DOWN:  new_head.y++; break;
        case LEFT:  new_head.x--; break;
        case RIGHT: new_head.x++; break;
    }

    /* 撞墙检测 */
    if (new_head.x < 0 || new_head.x >= WIDTH ||
        new_head.y < 0 || new_head.y >= HEIGHT) {
        game_over = 1;
        return;
    }

    /* 撞自身检测 */
    for (int i = 0; i < snake_len; i++) {
        if (snake[i].x == new_head.x && snake[i].y == new_head.y) {
            game_over = 1;
            return;
        }
    }

    /* 吃到食物 */
    int ate = (new_head.x == food.x && new_head.y == food.y);

    /* 移动：整体后移一格 */
    int move_len = ate ? snake_len : snake_len - 1;
    memmove(&snake[1], &snake[0], move_len * sizeof(Point));
    snake[0] = new_head;

    if (ate) {
        snake_len++;
        score += 10;
        if (snake_len < MAX_LEN) place_food();
        else game_over = 1; /* 通关 */
    }
}

/* -------- 主函数 -------- */
int main() {
    srand((unsigned)time(NULL));

#ifndef _WIN32
    enable_raw_mode();
#endif

    init_game();

    while (!game_over) {
        draw();
        handle_input();
        update();
        SLEEP(150);
    }

    draw();
    printf("\n  ══════════════════════\n");
    printf("       游戏结束！\n");
    printf("    最终得分: %d\n", score);
    printf("  ══════════════════════\n\n");

#ifndef _WIN32
    disable_raw_mode();
#endif

    return 0;
}
