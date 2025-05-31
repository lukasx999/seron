#include <stdio.h>

#include <raylib.h>

#define WIDTH 1600
#define HEIGHT 900
#define CELL_COUNT 300
#define CELL_SIZE 5
#define CELL_ITERS 1000

static int iter = 1;
static bool cells[CELL_ITERS][CELL_COUNT] = { 0 };

void draw_cells(void) {
    for (size_t i=0; i < CELL_ITERS; ++i) {
        for (size_t j=0; j < CELL_COUNT; ++j) {
            if (cells[i][j]) {
                DrawRectangle(
                    j * CELL_SIZE,
                    i * CELL_SIZE,
                    CELL_SIZE,
                    CELL_SIZE,
                    BLUE
                );

            }
        }
    }
}

bool rule110(bool left, bool center, bool right) {
    return
     left &&  center &&  right ? 0 :
     left &&  center && !right ? 1 :
     left && !center &&  right ? 1 :
     left && !center && !right ? 0 :
    !left &&  center &&  right ? 1 :
    !left &&  center && !right ? 1 :
    !left && !center &&  right ? 1 :
    !left && !center && !right ? 0
    : 0;
}

void next(void) {

    if (iter >= CELL_ITERS) return;

    bool *c = cells[iter-1];
    for (size_t i=0; i < CELL_COUNT; ++i) {

        bool left = i == 0
            ? c[CELL_COUNT-1]
            : c[i-1];

        bool right = i == CELL_COUNT-1
            ? c[0]
            : c[i+1];

        cells[iter][i] = rule110(left, c[i], right);
    }

    iter++;
}

int main(void) {

    cells[0][CELL_COUNT-1] = 1;

    SetTargetFPS(60);
    InitWindow(WIDTH, HEIGHT, "rule 110 C implementation");

    while (!WindowShouldClose()) {
        BeginDrawing();
        {
            ClearBackground(BLACK);
            draw_cells();
            next();

        }
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
