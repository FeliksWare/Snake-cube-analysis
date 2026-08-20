#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#define SNAKE_MAX (0x1FFFFFF)
#define CUBE_FULL (0x7FFFFFF)

typedef uint32_t Cube; // 27 bits
typedef uint32_t Snake; // 25 bits

typedef enum {
    X_POS,
    X_NEG,
    Y_POS,
    Y_NEG,
    Z_POS,
    Z_NEG,
} Direction;

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
        if ((snake >> i) & 1) {
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
        printf("%d", (snake >> i) & 1);
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

        if ((snake >> i) & 1) {
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

void fill_cube_end(size_t *solutions, Snake snake) {
    assert(snake_valid(snake));

    solutions[snake] += 1;
}

void fill_cube(size_t* solutions, Cube cube, Snake snake, size_t index, int x, int y, int z, Direction direction) {
    if (snake > flip_snake(snake)) return;
    move_direction(direction, &x, &y, &z);
    if (!cube_valid_pos(x, y, z)) return;
    if (cube_get(cube, x, y, z)) return;
    cube_set(&cube, x, y, z);

    if (index == 25) {
        assert(cube == CUBE_FULL);
        fill_cube_end(solutions, snake);
    }

    fill_cube(solutions, cube, snake | ((direction != X_POS) << index), index + 1, x, y, z, X_POS);
    fill_cube(solutions, cube, snake | ((direction != X_NEG) << index), index + 1, x, y, z, X_NEG);
    fill_cube(solutions, cube, snake | ((direction != Y_POS) << index), index + 1, x, y, z, Y_POS);
    fill_cube(solutions, cube, snake | ((direction != Y_NEG) << index), index + 1, x, y, z, Y_NEG);
    fill_cube(solutions, cube, snake | ((direction != Z_POS) << index), index + 1, x, y, z, Z_POS);
    fill_cube(solutions, cube, snake | ((direction != Z_NEG) << index), index + 1, x, y, z, Z_NEG);
}

void fill_cube_start(size_t *solutions, int x, int y, int z, Direction direction) {
    Cube cube = 0;
    Snake snake = 0;

    cube_set(&cube, 0, 0, 0);
    fill_cube(solutions, cube, snake, 0, x, y, z, direction);
}

size_t* fill_cube_count_solutions(void) {
    size_t *solutions = calloc(SNAKE_MAX, sizeof(*solutions));

    fill_cube_start(solutions, 0, 0, 0, X_POS);
    fill_cube_start(solutions, 1, 0, 0, X_NEG);
    fill_cube_start(solutions, 1, 0, 0, Y_POS);
    fill_cube_start(solutions, 1, 1, 0, Y_NEG);
    fill_cube_start(solutions, 1, 1, 0, Z_POS);
    fill_cube_start(solutions, 1, 1, 1, Z_NEG);

    return solutions;
}

int main(void) {
    size_t *solutions = fill_cube_count_solutions();

    for (Snake snake = 0; snake <= SNAKE_MAX; snake++) {
        if (!snake_valid(snake)) continue;

        print_snake(snake);
        printf("Solutions: %zu\n\n", solutions[snake]);
    }

    return 0;
}
