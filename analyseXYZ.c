#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#define CUBE_WIDTH  (3)
#define CUBE_HEIGHT (3)
#define CUBE_DEPTH  (3)

#define GRID_WIDTH  (2 * CUBE_WIDTH  - 1)
#define GRID_HEIGHT (2 * CUBE_HEIGHT - 1)
#define GRID_DEPTH  (2 * CUBE_DEPTH  - 1)

#define AXIS_MASK (X_AXIS | Y_AXIS | Z_AXIS)
#define NEG_MASK  (1 << 3)

#define SNAKE_LENGTH      (CUBE_WIDTH * CUBE_HEIGHT * CUBE_DEPTH - 2)
#define SOLUTION_LENGTH   (CUBE_WIDTH * CUBE_HEIGHT * CUBE_DEPTH - 1)
#define GRID_SIZE         (GRID_WIDTH * GRID_HEIGHT * GRID_DEPTH)
#define GRID_ARRAY_LENGTH ((GRID_SIZE + 63) / 64)

#define DIRECTION_COUNT (6)

#define SNAKE_MIN (0x0000000)
#define SNAKE_MAX (~(Snake)0 << (64 - SNAKE_LENGTH) >> (64 - SNAKE_LENGTH))

typedef enum {
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

typedef uint64_t Snake;

typedef struct {
    uint8_t directions[SOLUTION_LENGTH];
} Solution;

typedef struct {
    uint64_t grid[GRID_ARRAY_LENGTH];
    int8_t min_x;
    int8_t max_x;
    int8_t min_y;
    int8_t max_y;
    int8_t min_z;
    int8_t max_z;
} Grid;

typedef struct {
    size_t capacity;
    size_t count;
    Solution *elements;
} SolutionList;

typedef struct SolutionTree {
    struct SolutionTree *left;
    struct SolutionTree *right;
    Snake snake;
    SolutionList list;
} SolutionTree;

///////////////
// DIRECTION //
///////////////

void move_in_direction(Direction direction, int *x, int *y, int *z) {
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

Direction reverse_direction(Direction direction) {
    switch (direction) {
        case X_POS: return X_NEG;
        case X_NEG: return X_POS;
        case Y_POS: return Y_NEG;
        case Y_NEG: return Y_POS;
        case Z_POS: return Z_NEG;
        case Z_NEG: return Z_POS;
        default: assert(false && "Unreachable");
    }
}

///////////
// SNAKE //
///////////

bool is_snake_pos_valid(int pos) {
    return 0 <= pos && pos < SNAKE_LENGTH;
}

Snake snake_set(Snake snake, int pos) {
    assert(is_snake_pos_valid(pos));
    return snake | ((Snake)1 << pos);
}

bool snake_get(Snake snake, int pos) {
    assert(is_snake_pos_valid(pos));
    return (snake >> pos) & 1;
}

Snake flip_snake_naive(Snake snake) {
    Snake reverse = 0;

    for (size_t i = 0; i < SNAKE_LENGTH; i++) {
        reverse <<= 1;
        reverse |= snake & 1;
        snake >>= 1;
    }

    return reverse;
}

Snake flip_snake(Snake snake) {
    snake = (snake & 0xFFFFFFFF00000000) >> 32 | (snake & 0x00000000FFFFFFFF) << 32;
    snake = (snake & 0xFFFF0000FFFF0000) >> 16 | (snake & 0x0000FFFF0000FFFF) << 16;
    snake = (snake & 0xFF00FF00FF00FF00) >> 8  | (snake & 0x00FF00FF00FF00FF) << 8;
    snake = (snake & 0xF0F0F0F0F0F0F0F0) >> 4  | (snake & 0x0F0F0F0F0F0F0F0F) << 4;
    snake = (snake & 0xCCCCCCCCCCCCCCCC) >> 2  | (snake & 0x3333333333333333) << 2;
    snake = (snake & 0xAAAAAAAAAAAAAAAA) >> 1  | (snake & 0x5555555555555555) << 1;
    snake >>= 64 - SNAKE_LENGTH;

    return snake;
}

bool is_snake_valid(Snake snake) {
    if (snake > SNAKE_MAX) return false;
    if (snake > flip_snake(snake)) return false;

    int straight_length = 1;
    for (int i = 0; i < SNAKE_LENGTH; i++) {
        if (snake_get(snake, i)) {
            straight_length = 1;
        } else {
            straight_length += 1;
        }

        if (
            straight_length >= CUBE_WIDTH  &&
            straight_length >= CUBE_HEIGHT &&
            straight_length >= CUBE_DEPTH
        ) {
            return false;
        }
    }

    return true;
}

int snake_straight_count(Snake snake) {
    int count = 0;

    for (int i = 0; i < SNAKE_LENGTH; i++) {
        if (snake_get(snake, i)) continue;
        count++;
    }

    return count;
}

void print_snake(Snake snake) {
    putchar('-');
    for (int i = SNAKE_LENGTH - 1; i >= 0; i--) {
        putchar(snake_get(snake, i) ? 'o' : '-');
    }
    putchar('-');
    putchar('\n');
}

//////////////
// SOLUTION //
//////////////

Snake solution_to_snake(Solution solution) {
    Snake snake = 0;

    for (size_t i = 0; i < SNAKE_LENGTH; i++) {
        if (solution.directions[i] & solution.directions[i + 1] & AXIS_MASK) continue;
        snake = snake_set(snake, i);
    }

    return snake;
}

bool solutions_equal(Solution a, Solution b) {
    Snake snake_a = solution_to_snake(a);
    Snake snake_b = solution_to_snake(b);

    bool forward = snake_a == snake_b;
    bool reverse = snake_a == flip_snake(snake_b);

    if (!forward && !reverse) return false;

    uint8_t a_to_b[DIRECTION_COUNT] = {0};
    uint8_t b_to_a[DIRECTION_COUNT] = {0};

    uint8_t a_to_r[DIRECTION_COUNT] = {0};
    uint8_t r_to_a[DIRECTION_COUNT] = {0};

    for (size_t i = 0; i < SOLUTION_LENGTH; i++) {
        size_t index_a = direction_to_index(a.directions[i]);
        size_t index_b = direction_to_index(b.directions[i]);
        size_t index_r = direction_to_index(b.directions[SOLUTION_LENGTH - i - 1]);

        if (forward) {
            if (a_to_b[index_a] == 0) {
                a_to_b[index_a] = b.directions[i];
                a_to_b[direction_to_index(reverse_direction(a.directions[i]))] = reverse_direction(b.directions[i]);
            } else if (a_to_b[index_a] != b.directions[i]) {
                forward = false;
            }

            if (b_to_a[index_b] == 0) {
                b_to_a[index_b] = a.directions[i];
                b_to_a[direction_to_index(reverse_direction(b.directions[i]))] = reverse_direction(a.directions[i]);
            } else if (b_to_a[index_b] != a.directions[i]) {
                forward = false;
            }
        }

        if (reverse) {
            if (a_to_r[index_a] == 0) {
                a_to_r[index_a] = b.directions[SOLUTION_LENGTH - i - 1];
                a_to_r[direction_to_index(reverse_direction(a.directions[i]))] = reverse_direction(b.directions[SOLUTION_LENGTH - i - 1]);
            } else if (a_to_r[index_a] != b.directions[SOLUTION_LENGTH - i - 1]) {
                reverse = false;
            }

            if (r_to_a[index_r] == 0) {
                r_to_a[index_r] = a.directions[i];
                r_to_a[direction_to_index(reverse_direction(b.directions[SOLUTION_LENGTH - i - 1]))] = reverse_direction(a.directions[i]);
            } else if (r_to_a[index_r] != a.directions[i]) {
                reverse = false;
            }
        }

        if (!forward && !reverse) return false;
    }

    return true;
}

void print_solution(Solution solution) {
    for (size_t i = 0; i < SOLUTION_LENGTH; i++) {
        printf("%s ", direction_to_string(solution.directions[i]));
    }
    printf("\n");
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
    for (size_t i = 0; i < list->count; i++) {
        if (solutions_equal(list->elements[i], solution)) return;
    }

    solution_list_reserve(list, list->count + 1);
    list->elements[list->count++] = solution;
}

void print_solution_list(const SolutionList *list) {
    for (size_t i = 0; i < list->count; i++) {
        printf("Solution #%zu:\n", i + 1);
        print_solution(list->elements[i]);
        printf("\n");
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
        .list = {0},
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

bool is_grid_pos_valid(int x, int y, int z) {
    return 0 <= x && x < GRID_WIDTH  &&
           0 <= y && y < GRID_HEIGHT &&
           0 <= z && z < GRID_DEPTH;
}

void grid_set(Grid *grid, int x, int y, int z) {
    assert(is_grid_pos_valid(x, y, z));

    if (x < grid->min_x) grid->min_x = x;
    if (x > grid->max_x) grid->max_x = x;
    if (y < grid->min_y) grid->min_y = y;
    if (y > grid->max_y) grid->max_y = y;
    if (z < grid->min_z) grid->min_z = z;
    if (z > grid->max_z) grid->max_z = z;

    int i = GRID_WIDTH * GRID_HEIGHT * z + GRID_WIDTH * y + x;
    grid->grid[i / 64] |= (uint64_t)1 << (i % 64);
}

bool grid_get(Grid grid, int x, int y, int z) {
    assert(is_grid_pos_valid(x, y, z));
    int i = GRID_WIDTH * GRID_HEIGHT * z + GRID_WIDTH * y + x;
    return (grid.grid[i / 64] >> (i % 64)) & 1;
}

bool is_snake_cube_possible(Grid grid) {
    return grid.max_x - grid.min_x < CUBE_WIDTH  &&
           grid.max_y - grid.min_y < CUBE_HEIGHT &&
           grid.max_z - grid.min_z < CUBE_DEPTH;
}

Axis solution_symmetry(Grid grid, Axis symmetry, int px, int py, int pz) {
    for (int z = grid.min_z; z <= grid.max_z; z++) {
        for (int y = grid.min_y; y <= grid.max_y; y++) {
            for (int x = grid.min_x; x <= grid.max_x; x++) {
                if (symmetry == 0) return 0;

                if (!grid_get(grid, x, y, z)) continue;

                if ((symmetry & X_AXIS) && x != px) symmetry &= ~X_AXIS;
                if ((symmetry & Y_AXIS) && y != py) symmetry &= ~Y_AXIS;
                if ((symmetry & Z_AXIS) && z != pz) symmetry &= ~Z_AXIS;
            }
        }
    }

    return symmetry;
}

//////////////
// ANALYSIS //
//////////////

void fill_grid_impl(SolutionTree **tree, Solution solution, Grid grid, Axis symmetry, Snake snake, int pos, int x, int y, int z, Direction direction) {
    if (snake > flip_snake(snake)) {
        assert(!is_snake_valid(snake));
        return;
    }
    move_in_direction(direction, &x, &y, &z);
    if (!is_grid_pos_valid(x, y, z)) return;
    if (grid_get(grid, x, y, z)) return;

    grid_set(&grid, x, y, z);
    if (!is_snake_cube_possible(grid)) return;

    solution.directions[pos] = direction;

    if (pos == SNAKE_LENGTH) {
        assert(is_snake_valid(snake));
        solution_tree_add(tree, snake, solution);
        return;
    }

    symmetry = solution_symmetry(grid, symmetry, x, y, z);
    Axis axis = 0;

    fill_grid_impl(tree, solution, grid, symmetry, snake, pos + 1, x, y, z, direction);

    for (size_t i = 0; i < DIRECTION_COUNT; i++) {
        Direction next_direction = index_to_direction(i);
        Axis next_axis = next_direction & AXIS_MASK;

        if ((direction & next_axis) == 0 && ((symmetry & axis) == 0 || (symmetry & next_axis) == 0)) {
            fill_grid_impl(tree, solution, grid, symmetry, snake_set(snake, pos), pos + 1, x, y, z, next_direction);
            axis |= next_axis;
        }
    }
}

void fill_grid(SolutionTree **tree) {
    Grid grid;
    Solution solution;
    Axis symmetry = AXIS_MASK;
    Snake snake = SNAKE_MIN;
    int pos = 0;
    Direction direction = index_to_direction(0);

    int x = CUBE_WIDTH  - 1;
    int y = CUBE_HEIGHT - 1;
    int z = CUBE_DEPTH  - 1;

    init_grid(&grid);
    grid_set(&grid, x, y, z);

    fill_grid_impl(tree, solution, grid, symmetry, snake, pos, x, y, z, direction);
}

//////////////
// COMMANDS //
//////////////

void solutions_command(void) {
    SolutionTree *tree = NULL;
    fill_grid(&tree);
    
    for (Snake snake = 0; snake <= SNAKE_MAX; snake++) {
        if (!is_snake_valid(snake)) continue;

        SolutionList *list = solution_tree_get(tree, snake);

        if (list == NULL) continue;

        printf("\n\tSnake 0x%.*lX:\n", (SNAKE_LENGTH + 3) / 4, snake);
        printf("\t\t"); print_snake(snake);
        printf("\t\t"); print_snake(flip_snake(snake));
        printf("\n");
        print_solution_list(list);
    }

    free_solution_tree(&tree);
}

void print_table(SolutionTree *tree, bool palindrome, bool non_palindrome) {
    int max_straights = 0;
    size_t max_solutions = 0;

    for (Snake snake = 0; snake <= SNAKE_MAX; snake++) {
        if (!is_snake_valid(snake)) continue;
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
        if (!is_snake_valid(snake)) continue;
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
    printf("\n# None palindromic\n\n");
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
    fprintf(stderr, "    report       Print a table of all solutions\n");
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        usage(argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "solutions") == 0) {
        solutions_command();
    } else if (strcmp(argv[1], "report") == 0) {
        report_command();
    } else {
        fprintf(stderr, "ERROR: unkown command '%s'\n", argv[1]);
        usage(argv[0]);
    }

    return 0;
}
