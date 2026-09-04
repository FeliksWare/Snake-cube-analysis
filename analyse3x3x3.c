#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <limits.h>

#define ARRAY_LENGTH(x) (sizeof(x) / sizeof(x[0]))

#define GRID_WIDTH (5)
#define GRID_HEIGHT (5)
#define GRID_DEPTH (5)

#define SNAKE_MAX (0x1FFFFFF)

#define AXIS_MASK (X_AXIS | Y_AXIS | Z_AXIS)
#define NEG_MASK  (1 << 3)

#define DIRECTION_COUNT (6)

#define SOLUTION_LENGTH (26)

typedef uint32_t Snake; // 25 bits

typedef enum {
    NO_AXIS = 0,
    X_AXIS = 1 << 0,
    Y_AXIS = 1 << 1,
    Z_AXIS = 1 << 2,
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

typedef struct {
    uint8_t directions[SOLUTION_LENGTH];
} Solution;

typedef struct {
    Solution* elements;
    size_t capacity;
    size_t count;
} SolutionList;

typedef struct SolutionTree {
    struct SolutionTree *left;
    struct SolutionTree *right;
    Snake snake;
    SolutionList list;
} SolutionTree;

///////////
// SNAKE //
///////////

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
    printf("-");
    for (int i = 24; i >= 0; i--) {
        if (snake_get(snake, i)) {
            printf("o");
        } else {
            printf("-");
        }
    }
    printf("-\n");
}

size_t count_snakes(void) {
    size_t count = 0;

    for (Snake snake = 0; snake <= SNAKE_MAX; snake++) {
        if (!snake_valid(snake)) continue;
        count++;
    }

    return count;
}

int snake_straight_count(Snake snake) {
    int count = 0;

    for (int i = 0; i < 25; i++) {
        if (snake_get(snake, i)) continue;
        count++;
    }

    return count;
}

bool is_snake_palindrome(Snake snake) {
    return snake == flip_snake(snake);
}

///////////////
// DIRECTION //
///////////////

const char* direction_to_string(Direction direction) {
    switch (direction) {
        case X_POS: return "+x";
        case X_NEG: return "-x";
        case Y_POS: return "+y";
        case Y_NEG: return "-y";
        case Z_POS: return "+z";
        case Z_NEG: return "-z";
        default: assert(false && "Unreachable");
    }
}

size_t direction_to_index(Direction direction) {
    switch (direction) {
        case X_POS: return 0;
        case X_NEG: return 1;
        case Y_POS: return 2;
        case Y_NEG: return 3;
        case Z_POS: return 4;
        case Z_NEG: return 5;
        default: assert(false && "Unreachable");
    }
}

Direction index_to_direction(size_t index) {
    switch (index) {
        case 0: return X_POS;
        case 1: return X_NEG;
        case 2: return Y_POS;
        case 3: return Y_NEG;
        case 4: return Z_POS;
        case 5: return Z_NEG;
        default: assert(false && "Unreachable");
    }
}

void move_direction(Direction direction, int *x, int *y, int *z) {
    switch (direction) {
        case X_POS: *x += 1; break;
        case X_NEG: *x -= 1; break;
        case Y_POS: *y += 1; break;
        case Y_NEG: *y -= 1; break;
        case Z_POS: *z += 1; break;
        case Z_NEG: *z -= 1; break;
        default: assert(false && "Unreachable");
    }
}

//////////
// GRID //
//////////

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

Axis grid_symmetry(Grid grid, Axis symmetry, int px, int py, int pz) {
    for (int z = grid.min_z; z <= grid.max_z; z++) {
        for (int y = grid.min_y; y <= grid.max_y; y++) {
            for (int x = grid.min_x; x <= grid.max_x; x++) {
                if (symmetry == NO_AXIS) return NO_AXIS;

                if ((symmetry & X_AXIS) && (grid_get(grid, x, y, z) != grid_get_safe(grid, 2*px - x, y, z))) {
                    symmetry &= ~X_AXIS;
                }
                if ((symmetry & Y_AXIS) && (grid_get(grid, x, y, z) != grid_get_safe(grid, x, 2*py - y, z))) {
                    symmetry &= ~Y_AXIS;
                }
                if ((symmetry & Z_AXIS) && (grid_get(grid, x, y, z) != grid_get_safe(grid, x, y, 2*pz - z))) {
                    symmetry &= ~Z_AXIS;
                }
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

//////////////
// SOLUTION //
//////////////

void print_solution(Solution solution) {
    for (size_t i = 0; i < SOLUTION_LENGTH; i++) {
        printf("%s ", direction_to_string(solution.directions[i]));
    }
    printf("\n");
}

void print_solution_unique(Solution solution) {
    int directions[DIRECTION_COUNT];
    int count = 0;

    for (int i = 0; i < DIRECTION_COUNT; i++) {
        directions[i] = -1;
    }

    for (size_t i = 0; i < SOLUTION_LENGTH; i++) {
        size_t index = direction_to_index(solution.directions[i]);

        if (directions[index] == -1) {
            directions[index] = count++;
        }

        printf("%d", directions[index]);
    }
    printf("\n");
}

bool solution_equal_impl(Solution a, Solution b) {
    uint8_t a_to_b[DIRECTION_COUNT] = {0};
    uint8_t b_to_a[DIRECTION_COUNT] = {0};

    for (size_t i = 0; i < SOLUTION_LENGTH; i++) {
        size_t index_a = direction_to_index(a.directions[i]);
        size_t index_b = direction_to_index(b.directions[i]);

        if (a_to_b[index_a] == 0) {
            a_to_b[index_a] = b.directions[i];
        } else if (a_to_b[index_a] != b.directions[i]) {
            return false;
        }

        if (b_to_a[index_b] == 0) {
            b_to_a[index_b] = a.directions[i];
        } else if (b_to_a[index_b] != a.directions[i]) {
            return false;
        }
    }

    return true;
}

bool solution_equal(Solution a, Solution b) {
    if (solution_equal_impl(a, b)) return true;

    size_t l = 0;
    size_t r = SOLUTION_LENGTH - 1;

    while (l < r) {
        Direction t = b.directions[l];
        b.directions[l] = b.directions[r];
        b.directions[r] = t;

        l++;
        r--;
    }

    return solution_equal_impl(a, b);
}

///////////////////
// SOLUTION LIST //
///////////////////

void free_solution_list(SolutionList *list) {
    if (list->elements) {
        free(list->elements);
    }

    *list = (SolutionList){0};
}

void solution_list_reserve(SolutionList *list, size_t minimum) {
    if (list->capacity >= minimum) return;

    if (list->capacity == 0) {
        list->capacity = 8;
    }

    while (list->capacity < minimum) {
        list->capacity *= 2;
    }

    list->elements = realloc(list->elements, list->capacity * sizeof(*list->elements));
}

void solution_list_push(SolutionList *list, Solution solution) {
    solution_list_reserve(list, list->count + 1);
    list->elements[list->count++] = solution;
}

void print_solution_list(const SolutionList *list) {
    for (size_t i = 0; i < list->count; i++) {
        printf("Solution #%zu\n", i + 1);
        print_solution(list->elements[i]);
        printf("\n");
    }
}

void print_solution_list_unique(const SolutionList *list) {
    for (size_t i = 0; i < list->count; i++) {
        print_solution_unique(list->elements[i]);
    }
}

void solution_list_make_unique(SolutionList *list) {
    for (size_t i = 0; i + 1 < list->count; i++) {
        size_t j = i;
        for (size_t k = i + 1; k < list->count; k++) {
            if (solution_equal(list->elements[i], list->elements[k])) {
                continue;
            }

            j += 1;
            Solution t = list->elements[j];
            list->elements[j] = list->elements[k];
            list->elements[k] = t;
        }

        list->count = j + 1;
    }
}

///////////////////
// SOLUTION TREE //
///////////////////

SolutionTree* new_solution_tree(Snake snake) {
    SolutionTree *tree = malloc(sizeof(*tree));
    assert(tree != NULL);

    *tree = (SolutionTree){
        .left = NULL,
        .right = NULL,
        .snake = snake,
        .list = (SolutionList){0},
    };

    return tree;
}

void free_solution_tree(SolutionTree **tree) {
    if (*tree == NULL) return;

    free_solution_tree(&(*tree)->left);
    free_solution_tree(&(*tree)->right);
    free_solution_list(&(*tree)->list);
    free(*tree);
    *tree = NULL;
}

void solution_tree_add(SolutionTree **root, Snake snake, Solution solution) {
    while (*root != NULL) {
        if (snake < (*root)->snake) {
            root = &(*root)->left;
        } else if (snake > (*root)->snake) {
            root = &(*root)->right;
        } else {
            break;
        }
    }

    if (*root == NULL) {
        *root = new_solution_tree(snake);
    }

    solution_list_push(&(*root)->list, solution);
}

SolutionList* solution_tree_get(SolutionTree *root, Snake snake) {
    while (root != NULL) {
        if (snake < root->snake) {
            root = root->left;
        } else if (snake > root->snake) {
            root = root->right;
        } else {
            return &root->list;
        }
    }

    return NULL;
}

void solution_tree_make_unique(SolutionTree *root) {
    if (root == NULL) return;

    solution_list_make_unique(&root->list);
    solution_tree_make_unique(root->left);
    solution_tree_make_unique(root->right);
}

//////////////
// ANALYSIS //
//////////////

void fill_grid_impl(SolutionTree **tree, Solution solution, Grid grid, Axis symmetry, Snake snake, int pos, int x, int y, int z, Direction direction) {
    solution.directions[pos] = direction;
    if (snake > flip_snake(snake)) {
        assert(!snake_valid(snake));
        return;
    }
    move_direction(direction, &x, &y, &z);
    if (!grid_valid_pos(x, y, z)) return;
    if (grid_get(grid, x, y, z)) return;
    grid_set(&grid, x, y, z);

    if (!is_snake_solution_possible(grid)) return;

    if (pos == 25) {
        assert(snake_valid(snake));
        if (!is_snake_palindrome(snake)) {
            solution_tree_add(tree, snake, solution);
        } else {
            SolutionList *list = solution_tree_get(*tree, snake);

            if (list != NULL) {
                for (size_t i = 0; i < list->count; i++) {
                    if (solution_equal(list->elements[i], solution)) return;
                }
            }

            solution_tree_add(tree, snake, solution);
        }
        return;
    }

    symmetry = grid_symmetry(grid, symmetry, x, y, z);
    Axis axis = NO_AXIS;

    fill_grid_impl(tree, solution, grid, symmetry, snake, pos + 1, x, y, z, direction);

    for (size_t i = 0; i < DIRECTION_COUNT; i++) {
        Direction next_direction = index_to_direction(i);
        Axis next_axis = next_direction & AXIS_MASK;

        if ((direction & next_axis) == 0 && ((symmetry & axis) == 0 || (symmetry & next_axis) == 0)) {
            fill_grid_impl(tree, solution, grid, symmetry, snake_set(snake, pos, 1), pos + 1, x, y, z, next_direction);
            axis |= next_axis;
        }
    }
}

void fill_grid(SolutionTree **tree) {
    Solution solution;
    Grid grid;
    Snake snake = 0;

    init_grid(&grid);
    grid_set(&grid, 2, 2, 2);

    fill_grid_impl(tree, solution, grid, AXIS_MASK, snake, 0, 2, 2, 2, X_POS);
}

void solve_snake_impl(SolutionList *list, Solution solution, Grid grid, Axis symmetry, Snake snake, int pos, int x, int y, int z, Direction direction) {
    solution.directions[pos] = direction;
    move_direction(direction, &x, &y, &z);
    if (!grid_valid_pos(x, y, z)) return;
    if (grid_get(grid, x, y, z)) return;
    grid_set(&grid, x, y, z);

    if (!is_snake_solution_possible(grid)) return;

    if (pos == 25) {
        if (!is_snake_palindrome(snake)) {
            solution_list_push(list, solution);
        } else {
            if (list != NULL) {
                for (size_t i = 0; i < list->count; i++) {
                    if (solution_equal(list->elements[i], solution)) return;
                }
            }

            solution_list_push(list, solution);
        }
        return;
    }

    symmetry = grid_symmetry(grid, symmetry, x, y, z);

    if (snake_get(snake, pos)) {
        Axis axis = NO_AXIS;

        for (size_t i = 0; i < DIRECTION_COUNT; i++) {
            Direction next_direction = index_to_direction(i);
            Axis next_axis = next_direction & AXIS_MASK;

            if ((direction & next_axis) == 0 && ((symmetry & axis) == 0 || (symmetry & next_axis) == 0)) {
                solve_snake_impl(list, solution, grid, symmetry, snake_set(snake, pos, 1), pos + 1, x, y, z, next_direction);
                axis |= next_axis;
            }
        }
    } else {
        solve_snake_impl(list, solution, grid, symmetry, snake, pos + 1, x, y, z, direction);
    }
}

void solve_snake(SolutionList *list, Snake snake) {
    assert(snake_valid(snake));

    Solution solution;
    Grid grid;
    
    init_grid(&grid);
    grid_set(&grid, 2, 2, 2);

    solve_snake_impl(list, solution, grid, AXIS_MASK, snake, 0, 2, 2, 2, X_POS);
}

void fill_grid_all_impl(SolutionTree **tree, Solution solution, Grid grid, Snake snake, int pos, int x, int y, int z, Direction direction) {
    solution.directions[pos] = direction;
    if (snake > flip_snake(snake)) {
        assert(!snake_valid(snake));
        return;
    }
    move_direction(direction, &x, &y, &z);
    if (!grid_valid_pos(x, y, z)) return;
    if (grid_get(grid, x, y, z)) return;
    grid_set(&grid, x, y, z);

    if (!is_snake_solution_possible(grid)) return;

    if (pos == 25) {
        assert(snake_valid(snake));
        solution_tree_add(tree, snake, solution);
        return;
    }

    fill_grid_all_impl(tree, solution, grid, snake, pos + 1, x, y, z, direction);

    for (size_t i = 0; i < DIRECTION_COUNT; i++) {
        Direction next_direction = index_to_direction(i);
        Axis next_axis = next_direction & AXIS_MASK;

        if ((direction & next_axis) == 0) {
            fill_grid_all_impl(tree, solution, grid, snake_set(snake, pos, 1), pos + 1, x, y, z, next_direction);
        }
    }
}

void fill_grid_all(SolutionTree **tree) {
    Solution solution;
    Grid grid;
    Snake snake = 0;

    init_grid(&grid);
    grid_set(&grid, 2, 2, 2);

    fill_grid_all_impl(tree, solution, grid, snake, 0, 2, 2, 2, X_POS);
}

void solve_snake_all_impl(SolutionList *list, Solution solution, Grid grid, Snake snake, int pos, int x, int y, int z, Direction direction) {
    solution.directions[pos] = direction;
    move_direction(direction, &x, &y, &z);
    if (!grid_valid_pos(x, y, z)) return;
    if (grid_get(grid, x, y, z)) return;
    grid_set(&grid, x, y, z);

    if (!is_snake_solution_possible(grid)) return;

    if (pos == 25) {
        solution_list_push(list, solution);
        return;
    }

    if (snake_get(snake, pos)) {
        for (size_t i = 0; i < DIRECTION_COUNT; i++) {
            Direction next_direction = index_to_direction(i);
            Axis next_axis = next_direction & AXIS_MASK;

            if ((direction & next_axis) == 0) {
                solve_snake_all_impl(list, solution, grid, snake_set(snake, pos, 1), pos + 1, x, y, z, next_direction);
            }
        }
    } else {
        solve_snake_all_impl(list, solution, grid, snake, pos + 1, x, y, z, direction);
    }
}

void solve_snake_all(SolutionList *list, Snake snake) {
    assert(snake_valid(snake));

    Solution solution;
    Grid grid;
    
    init_grid(&grid);
    grid_set(&grid, 2, 2, 2);

    solve_snake_all_impl(list, solution, grid, snake, 0, 2, 2, 2, X_POS);
}

///////////
// TESTS //
///////////

bool TEST_flip_snake(void) {
    for (Snake snake = 0; snake <= SNAKE_MAX; snake++) {
        if (flip_snake(snake) != flip_snake_naive(snake)) return false;
    }

    return true;
}

bool TEST_same_solution_count(void) {
    SolutionTree *tree = NULL;
    fill_grid(&tree);

    for (Snake snake = 0; snake <= SNAKE_MAX; snake++) {
        if (!snake_valid(snake)) continue;

        SolutionList a = {0};
        solve_snake(&a, snake);
        size_t a_count = a.count;
        free_solution_list(&a);

        SolutionList *b = solution_tree_get(tree, snake);

        if (b == NULL) {
            if (a_count == 0) continue;
        } else if (b->count == a_count) {
            continue;
        }

        free_solution_tree(&tree);
        free_solution_list(&a);
        return false;
    }

    free_solution_tree(&tree);
    return true;
}

bool TEST_symmetry(void) {
    SolutionTree *tree_a = NULL;
    fill_grid_all(&tree_a);
    solution_tree_make_unique(tree_a);

    SolutionTree *tree_b = NULL;
    fill_grid(&tree_b);

    for (Snake snake = 0; snake <= SNAKE_MAX; snake++) {
        if (!snake_valid(snake)) continue;

        SolutionList *list_a = solution_tree_get(tree_a, snake);
        SolutionList *list_b = solution_tree_get(tree_b, snake);

        if (list_a == NULL && list_b == NULL) continue;
        if (list_a != NULL && list_b != NULL && list_a->count == list_b->count) continue;

        free_solution_tree(&tree_a);
        free_solution_tree(&tree_b);
        return false;
    }

    free_solution_tree(&tree_a);
    free_solution_tree(&tree_b);
    return true;
}

//////////////
// COMMANDS //
//////////////

void test_command(void) {
    struct {
        const char *name;
        bool (*test)(void);
    } tests[] = {
        {"flip snake", TEST_flip_snake},
        {"same solution count", TEST_same_solution_count},
        {"symmetry", TEST_symmetry},
    };

    for (size_t i = 0; i < ARRAY_LENGTH(tests); i++) {
        bool passed = tests[i].test();

        if (passed) {
            printf("[PASSED] ");
        } else {
            printf("[FAILED] ");
        }

        printf("%s\n", tests[i].name);
    }
}

void solutions_command(void) {
    SolutionTree *tree = NULL;
    fill_grid(&tree);
    
    for (Snake snake = 0; snake <= SNAKE_MAX; snake++) {
        if (!snake_valid(snake)) continue;

        SolutionList *list = solution_tree_get(tree, snake);

        if (list == NULL) continue;

        printf("\n\tSnake 0x%07X:\n", snake);
        printf("\t\t"); print_snake(snake);
        printf("\t\t"); print_snake(flip_snake(snake));
        printf("\n");
        print_solution_list(list);
    }

    free_solution_tree(&tree);
}

void compare_command(void) {
    SolutionTree *tree = NULL;
    fill_grid_all(&tree);
    solution_tree_make_unique(tree);
    
    for (Snake snake = 0; snake <= SNAKE_MAX; snake++) {
        if (!snake_valid(snake)) continue;

        SolutionList *list = solution_tree_get(tree, snake);
        if (list == NULL) continue;

        print_solution_list_unique(list);
    }

    free_solution_tree(&tree);
}

void print_table(SolutionTree *tree, bool palindrome, bool non_palindrome) {
    int max_straights = 0;
    size_t max_solutions = 0;

    for (Snake snake = 0; snake <= SNAKE_MAX; snake++) {
        if (!snake_valid(snake)) continue;
        if (palindrome && snake != flip_snake(snake)) continue;
        if (non_palindrome && snake == flip_snake(snake)) continue;

        SolutionList *list = solution_tree_get(tree, snake);
        if (list == NULL) continue;

        int straight_count = snake_straight_count(snake);

        if (list->count > max_solutions) max_solutions = list->count;
        if (straight_count > max_straights) max_straights = straight_count;
    }

    int *table = calloc((max_solutions + 3) * (max_straights + 3), sizeof(*table));
    assert(table != NULL);

    for (Snake snake = 0; snake <= SNAKE_MAX; snake++) {
        if (!snake_valid(snake)) continue;
        if (palindrome && snake != flip_snake(snake)) continue;
        if (non_palindrome && snake == flip_snake(snake)) continue;

        SolutionList *list = solution_tree_get(tree, snake);
        if (list == NULL) continue;

        size_t solution_count = list->count;
        int straight_count = snake_straight_count(snake);

        table[(max_straights + 3) * solution_count + straight_count] += 1;

        table[(max_straights + 3) * solution_count + (max_straights + 1)] += 1;
        table[(max_straights + 3) * solution_count + (max_straights + 2)] += solution_count;

        table[(max_straights + 3) * (max_solutions + 1) + straight_count] += 1;
        table[(max_straights + 3) * (max_solutions + 2) + straight_count] += solution_count;

        table[(max_straights + 3) * (max_solutions + 1) + (max_straights + 1)] += 1;
        table[(max_straights + 3) * (max_solutions + 2) + (max_straights + 2)] += solution_count;
    }

    printf("|Solutions");
    for (int straight_count = 0; straight_count <= max_straights; straight_count++) {
        if (table[(max_straights + 3) * (max_solutions + 1) + straight_count] == 0) continue;
        printf("|%d", straight_count);
    }
    printf("|Total Snakes");
    printf("|Total Solutions");
    printf("|\n");

    printf("|-:");
    for (int straight_count = 0; straight_count <= max_straights; straight_count++) {
        if (table[(max_straights + 3) * (max_solutions + 1) + straight_count] == 0) continue;
        printf("|-:");
    }
    printf("|-:");
    printf("|-:");
    printf("|\n");

    for (size_t solution_count = 0; solution_count <= max_solutions; solution_count++) {
        if (table[(max_straights + 3) * solution_count + (max_straights + 1)] == 0) continue;
        printf("|%d", (int)solution_count);

        for (int straight_count = 0; straight_count <= max_straights; straight_count++) {
            if (table[(max_straights + 3) * (max_solutions + 1) + straight_count] == 0) continue;
            printf("|%d", table[(max_straights + 3) * solution_count + straight_count]);
        }
        printf("|%d", table[(max_straights + 3) * solution_count + (max_straights + 1)]);
        printf("|%d", table[(max_straights + 3) * solution_count + (max_straights + 2)]);
        printf("|\n");
    }

    printf("|Total Snakes");
    for (int straight_count = 0; straight_count <= max_straights; straight_count++) {
        if (table[(max_straights + 3) * (max_solutions + 1) + straight_count] == 0) continue;
        printf("|%d", table[(max_straights + 3) * (max_solutions + 1) + straight_count]);
    }
    printf("|%d", table[(max_straights + 3) * (max_solutions + 1) + (max_straights + 1)]);
    printf("|-");
    printf("|\n");

    printf("|Total Solutions");
    for (int straight_count = 0; straight_count <= max_straights; straight_count++) {
        if (table[(max_straights + 3) * (max_solutions + 1) + straight_count] == 0) continue;
        printf("|%d", table[(max_straights + 3) * (max_solutions + 2) + straight_count]);
    }
    printf("|-");
    printf("|%d", table[(max_straights + 3) * (max_solutions + 2) + (max_straights + 2)]);
    printf("|\n");

    free(table);
}

void report_command(void) {
    SolutionTree *tree = NULL;
    fill_grid(&tree);

    printf("# All\n\n");
    print_table(tree, false, false);
    printf("\n# Non-palindromic\n\n");
    print_table(tree, false, true);
    printf("\n# Palindromic\n\n");
    print_table(tree, true, false);

    free_solution_tree(&tree);
}

void usage(const char *program) {
    fprintf(stderr, "Usage: %s [Command]\n", program);
    fprintf(stderr, "Command:\n");
    fprintf(stderr, "    test         Run some regression tests\n");
    fprintf(stderr, "    solutions    Print all the solutions to all solvable snakes\n");
    fprintf(stderr, "    compare      Print solutions in a format that is easy to compare\n");
    fprintf(stderr, "    report       Print a table of all solutions\n");
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        usage(argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "test") == 0) {
        test_command();
    } else if (strcmp(argv[1], "solutions") == 0) {
        solutions_command();
    } else if (strcmp(argv[1], "compare") == 0) {
        compare_command();
    } else if (strcmp(argv[1], "report") == 0) {
        report_command();
    } else {
        fprintf(stderr, "ERROR: unkown command '%s'\n", argv[1]);
        usage(argv[0]);
    }

    return 0;
}
