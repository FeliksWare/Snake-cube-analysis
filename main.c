#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#define GRID_WIDTH (5)
#define GRID_HEIGHT (5)
#define GRID_DEPTH (5)

#define SNAKE_MAX (0x1FFFFFF)
#define CUBE_FULL (0x7FFFFFF)

#define AXIS_MASK (X_AXIS | Y_AXIS | Z_AXIS)
#define NEG_MASK  (1 << 3)

typedef uint32_t Cube; // 27 bits
typedef uint32_t Snake; // 25 bits

typedef enum {
    NO_AXIS = 0,
    X_AXIS = 1 << 0,
    Y_AXIS = 1 << 1,
    Z_AXIS = 1 << 2,
    XY_AXIS = X_AXIS | Y_AXIS,
    YZ_AXIS = Y_AXIS | Z_AXIS,
    XZ_AXIS = X_AXIS | Z_AXIS,
    XYZ_AXIS = X_AXIS | Y_AXIS | Z_AXIS,
} Axis;

typedef enum {
    X_POS = X_AXIS,
    X_NEG = X_AXIS | NEG_MASK,
    Y_POS = Y_AXIS,
    Y_NEG = Y_AXIS | NEG_MASK,
    Z_POS = Z_AXIS,
    Z_NEG = Z_AXIS | NEG_MASK,
} Direction;

typedef struct {
    uint64_t grid[6];
    int8_t min_x;
    int8_t max_x;
    int8_t min_y;
    int8_t max_y;
    int8_t min_z;
    int8_t max_z;
} Grid;

bool snake_valid_pos(int pos) {
    return 0 <= pos && pos < 25;
}

Snake snake_set(Snake snake, int pos, bool value) {
    assert(snake_valid_pos(pos));
    if (value) return snake | (1 << pos);
    return snake;
}

bool snake_get(Snake snake, int pos) {
    return (snake >> pos) & 1;
}

bool cube_valid_pos(int x, int y, int z) {
    return 0 <= x && x < 3 &&
           0 <= y && y < 3 &&
           0 <= z && z < 3;
}

void cube_set(Cube *cube, int x, int y, int z) {
    assert(cube_valid_pos(x, y, z));
    *cube |= 1 << (3*3*z + 3*y + x);
}

bool cube_get(Cube cube, int x, int y, int z) {
    assert(cube_valid_pos(x, y, z));
    return (cube >> (3*3*z + 3*y + x)) & 1;
}

void init_grid(Grid *grid) {
    *grid = (Grid){
        .grid = {0},
        .min_x = GRID_WIDTH - 1,
        .max_x = 0,
        .min_y = GRID_HEIGHT - 1,
        .max_y = 0,
        .min_z = GRID_DEPTH - 1,
        .max_z = 0,
    };
}

bool grid_valid_pos(int x, int y, int z) {
    return 0 <= x && x < GRID_WIDTH &&
           0 <= y && y < GRID_HEIGHT &&
           0 <= z && z < GRID_DEPTH;
}

void grid_set(Grid *grid, int x, int y, int z) {
    assert(grid_valid_pos(x, y, z));

    if (x < grid->min_x) grid->min_x = x;
    if (x > grid->max_x) grid->max_x = x;
    if (y < grid->min_y) grid->min_y = y;
    if (y > grid->max_y) grid->max_y = y;
    if (z < grid->min_z) grid->min_z = z;
    if (z > grid->max_z) grid->max_z = z;

    int i = GRID_WIDTH * GRID_HEIGHT * z + GRID_WIDTH * y + x;
    grid->grid[i % 6] |= 1 << (i / 6);
}

bool grid_get(Grid grid, int x, int y, int z) {
    assert(grid_valid_pos(x, y, z));
    int i = GRID_WIDTH * GRID_HEIGHT * z + GRID_WIDTH * y + x;
    return (grid.grid[i % 6] >> (i / 6)) & 1;
}

bool grid_get_safe(Grid grid, int x, int y, int z) {
    if (!grid_valid_pos(x, y, z)) return 0;
    return grid_get(grid, x, y, z);
}

Axis grid_symmetry(Grid grid, int px, int py, int pz) {
    Axis symmetry = XYZ_AXIS;

    for (int z = grid.min_z; z <= grid.max_z; z++) {
        for (int y = grid.min_y; y <= grid.max_y; y++) {
            for (int x = grid.min_x; x <= grid.max_x; x++) {
                if ((symmetry & X_AXIS) && (grid_get(grid, x, y, z) != grid_get_safe(grid, 2*px - x, y, z))) {
                    symmetry &= ~X_AXIS;
                }
                if ((symmetry & Y_AXIS) && (grid_get(grid, x, y, z) != grid_get_safe(grid, x, 2*py - y, z))) {
                    symmetry &= ~Y_AXIS;
                }
                if ((symmetry & Z_AXIS) && (grid_get(grid, x, y, z) != grid_get_safe(grid, x, y, 2*pz - z))) {
                    symmetry &= ~Z_AXIS;
                }

                if (symmetry == NO_AXIS) return NO_AXIS;
            }
        }
    }

    return symmetry;
}

void print_grid(Grid grid) {
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int z = 0; z < GRID_DEPTH; z++) {
            for (int x = 0; x < GRID_WIDTH; x++) {
                if (grid_get(grid, x, y, z)) {
                    printf("# ");
                } else {
                    printf(". ");
                }
            }
            printf("   ");
        }
        printf("\n");
    }
    printf("\n");
}

bool is_snake_solution_possible(Grid grid) {
    return grid.max_x - grid.min_x <= 2 &&
           grid.max_y - grid.min_y <= 2 &&
           grid.max_z - grid.min_z <= 2;
}

Snake flip_snake_naive(Snake snake) {
    return (snake & 0x1000000) >> 24 |
           (snake & 0x0800000) >> 22 |
           (snake & 0x0400000) >> 20 |
           (snake & 0x0200000) >> 18 |
           (snake & 0x0100000) >> 16 |
           (snake & 0x0080000) >> 14 |
           (snake & 0x0040000) >> 12 |
           (snake & 0x0020000) >> 10 |
           (snake & 0x0010000) >>  8 |
           (snake & 0x0008000) >>  6 |
           (snake & 0x0004000) >>  4 |
           (snake & 0x0002000) >>  2 |
           (snake & 0x0001000)       |
           (snake & 0x0000800) <<  2 |
           (snake & 0x0000400) <<  4 |
           (snake & 0x0000200) <<  6 |
           (snake & 0x0000100) <<  8 |
           (snake & 0x0000080) << 10 |
           (snake & 0x0000040) << 12 |
           (snake & 0x0000020) << 14 |
           (snake & 0x0000010) << 16 |
           (snake & 0x0000008) << 18 |
           (snake & 0x0000004) << 20 |
           (snake & 0x0000002) << 22 |
           (snake & 0x0000001) << 24;
}

Snake flip_snake(Snake snake) {
    // xxxxxxx1 11111111 111P0000 00000000 [0b1111111111110000000000000]
    // xxxxxxx0 00000000 000P1111 11111111 [0b0000000000000111111111111]
    // xxxxxxx0 00000000 00010000 00000000
    snake = (snake & 0x01FFE000) >> 13 | (snake & 0x00000FFF) << 13 | (snake & 0x00001000);
    // xxxxxxx1 11111000 000P1111 11000000 [0b1111110000000111111000000]
    // xxxxxxx0 00000111 111P0000 00111111 [0b0000001111110000000111111]
    // xxxxxxx0 00000000 00010000 00000000
    snake = (snake & 0x01F80FC0) >> 6  | (snake & 0x0007E03F) << 6  | (snake & 0x00001000);
    // xxxxxxx1 11000111 000P1110 00111000 [0b1110001110000111000111000]
    // xxxxxxx0 00111000 111P0001 11000111 [0b0001110001110000111000111]
    // xxxxxxx0 00000000 00010000 00000000
    snake = (snake & 0x01C70E38) >> 3  | (snake & 0x0038E1C7) << 3  | (snake & 0x00001000);
    // xxxxxxx1 p01p01p0 1p0P1p01 p01p01p0 [0b1001001001000100100100100]
    // xxxxxxx0 p10p10p1 0p1P0p10 p10p10p1 [0b0010010010010001001001001]
    // xxxxxxx0 10010010 01010100 10010010
    snake = (snake & 0x01248924) >> 2  | (snake & 0x00492249) << 2  | (snake & 0x00925492);

    return snake;
}

bool snake_valid(Snake snake) {
    if (snake > SNAKE_MAX) return false;
    if (snake > flip_snake(snake)) return false;

    int zeros = 0;
    for (int i = 0; i < 25; i++) {
        if (snake_get(snake, i)) {
            zeros = 0;
        } else {
            zeros++;
        }

        if (zeros > 1) return false;
    }

    return true;
}

void print_snake(Snake snake) {
    printf("(0)");
    for (int i = 0; i < 25; i++) {
        printf("%d", snake_get(snake, i));
    }
    printf("(0)");

    int length = 0;

    bool vertical = true;

    printf("\n#");

    for (int i = 0; i < 25; i++) {
        if (vertical) {
            printf("#");
            length += 1;
        } else {
            printf("\n");
            for (int i = 0; i < length; i++) printf(" ");
            printf("#");
        }

        if (snake_get(snake, i)) {
            vertical = !vertical;
        }
    }

    if (vertical) {
        printf("#");
    } else {
        printf("\n");
        for (int i = 0; i < length; i++) printf(" ");
        printf("#");
    }

    printf("\n");
}

size_t count_snakes(void) {
    size_t count = 0;

    for (Snake snake = 0; snake <= SNAKE_MAX; snake++) {
        if (!snake_valid(snake)) continue;
        count++;
    }

    return count;
}

bool TEST_flip_snake(void) {
    for (Snake snake = 0; snake <= SNAKE_MAX; snake++) {
        if (flip_snake(snake) != flip_snake_naive(snake)) return false;
    }

    return true;
}

void move_direction(Direction direction, int *x, int *y, int *z) {
    switch (direction) {
        case X_POS: *x += 1; break;
        case X_NEG: *x -= 1; break;
        case Y_POS: *y += 1; break;
        case Y_NEG: *y -= 1; break;
        case Z_POS: *z += 1; break;
        case Z_NEG: *z -= 1; break;
    }
}

void fill_grid_impl(size_t *solutions, Grid grid, Snake snake, int pos, int x, int y, int z, Direction direction) {
    if (snake > flip_snake(snake)) return;
    move_direction(direction, &x, &y, &z);
    if (!grid_valid_pos(x, y, z)) return;
    if (grid_get(grid, x, y, z)) return;
    grid_set(&grid, x, y, z);

    if (!is_snake_solution_possible(grid)) return;

    if (pos == 25) {
        assert(snake_valid(snake));
        solutions[snake] += 1;
        return;
    }

    Axis symmetry = grid_symmetry(grid, x, y, z);
    Axis axis = NO_AXIS;

    fill_grid_impl(solutions, grid, snake_set(snake, pos, 0), pos + 1, x, y, z, direction);

    if ((direction & X_AXIS) == 0 && ((symmetry & axis) == 0 || (symmetry & X_AXIS) == 0)) {
        fill_grid_impl(solutions, grid, snake_set(snake, pos, 1), pos + 1, x, y, z, X_POS);
        axis |= X_AXIS;
    }
    if ((direction & X_AXIS) == 0 && ((symmetry & axis) == 0 || (symmetry & X_AXIS) == 0)) {
        fill_grid_impl(solutions, grid, snake_set(snake, pos, 1), pos + 1, x, y, z, X_NEG);
        axis |= X_AXIS;
    }

    if ((direction & Y_AXIS) == 0 && ((symmetry & axis) == 0 || (symmetry & Y_AXIS) == 0)) {
        fill_grid_impl(solutions, grid, snake_set(snake, pos, 1), pos + 1, x, y, z, Y_POS);
        axis |= Y_AXIS;
    }
    if ((direction & Y_AXIS) == 0 && ((symmetry & axis) == 0 || (symmetry & Y_AXIS) == 0)) {
        fill_grid_impl(solutions, grid, snake_set(snake, pos, 1), pos + 1, x, y, z, Y_NEG);
        axis |= Y_AXIS;
    }

    if ((direction & Z_AXIS) == 0 && ((symmetry & axis) == 0 || (symmetry & Z_AXIS) == 0)) {
        fill_grid_impl(solutions, grid, snake_set(snake, pos, 1), pos + 1, x, y, z, Z_POS);
        axis |= Z_AXIS;
    }
    if ((direction & Z_AXIS) == 0 && ((symmetry & axis) == 0 || (symmetry & Z_AXIS) == 0)) {
        fill_grid_impl(solutions, grid, snake_set(snake, pos, 1), pos + 1, x, y, z, Z_NEG);
        axis |= Z_AXIS;
    }
}

size_t* fill_grid(void) {
    size_t *solutions = calloc(SNAKE_MAX, sizeof(*solutions));

    Grid grid;
    Snake snake = 0;

    init_grid(&grid);
    grid_set(&grid, 2, 2, 2);

    fill_grid_impl(solutions, grid, snake, 0, 2, 2, 2, X_POS);

    return solutions;
}

size_t solve_snake_impl(Grid grid, Snake snake, int pos, int x, int y, int z, Direction direction) {
    move_direction(direction, &x, &y, &z);
    if (!grid_valid_pos(x, y, z)) return 0;
    if (grid_get(grid, x, y, z)) return 0;
    grid_set(&grid, x, y, z);

    if (!is_snake_solution_possible(grid)) return 0;

    if (pos == 25) return 1;

    if (snake_get(snake, pos)) {
        size_t solutions = 0;

        Axis symmetry = grid_symmetry(grid, x, y, z);
        Axis axis = NO_AXIS;

        if ((direction & X_AXIS) == 0 && ((symmetry & axis) == 0 || (symmetry & X_AXIS) == 0)) {
            solutions += solve_snake_impl(grid, snake, pos + 1, x, y, z, X_POS);
            axis |= X_AXIS;
        }
        if ((direction & X_AXIS) == 0 && ((symmetry & axis) == 0 || (symmetry & X_AXIS) == 0)) {
            solutions += solve_snake_impl(grid, snake, pos + 1, x, y, z, X_NEG);
            axis |= X_AXIS;
        }

        if ((direction & Y_AXIS) == 0 && ((symmetry & axis) == 0 || (symmetry & Y_AXIS) == 0)) {
            solutions += solve_snake_impl(grid, snake, pos + 1, x, y, z, Y_POS);
            axis |= Y_AXIS;
        }
        if ((direction & Y_AXIS) == 0 && ((symmetry & axis) == 0 || (symmetry & Y_AXIS) == 0)) {
            solutions += solve_snake_impl(grid, snake, pos + 1, x, y, z, Y_NEG);
            axis |= Y_AXIS;
        }

        if ((direction & Z_AXIS) == 0 && ((symmetry & axis) == 0 || (symmetry & Z_AXIS) == 0)) {
            solutions += solve_snake_impl(grid, snake, pos + 1, x, y, z, Z_POS);
            axis |= Z_AXIS;
        }
        if ((direction & Z_AXIS) == 0 && ((symmetry & axis) == 0 || (symmetry & Z_AXIS) == 0)) {
            solutions += solve_snake_impl(grid, snake, pos + 1, x, y, z, Z_NEG);
            axis |= Z_AXIS;
        }

        return solutions;
    } else {
        return solve_snake_impl(grid, snake, pos + 1, x, y, z, direction);
    }
}

size_t solve_snake_count_solutions(Snake snake) {
    assert(snake_valid(snake));

    Grid grid;
    
    init_grid(&grid);
    grid_set(&grid, 2, 2, 2);

    return solve_snake_impl(grid, snake, 0, 2, 2, 2, X_POS);
}

int main(void) {
    size_t *solutions = fill_grid();

    for (Snake snake = 0; snake <= SNAKE_MAX; snake++) {
        if (!snake_valid(snake)) continue;

        printf("0x%X\n", snake);
        print_snake(snake);
        printf("Solutions: %zu\n", solutions[snake]);
        printf("Solves: %zu\n\n", solve_snake_count_solutions(snake));
        solve_snake_count_solutions(snake);
    }

    free(solutions);

    return 0;
}
