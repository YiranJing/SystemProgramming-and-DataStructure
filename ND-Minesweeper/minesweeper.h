#include "params.h"

struct cell {
    int mined;
    int selected;
    int num_adjacent;
    struct cell * adjacent[MAX_ADJACENT];
    int coords[MAX_DIM];
    int hint;
};

void init_game(struct cell * game, int dim, int * dim_sizes, int num_mines, int ** mined_cells);

int select_cell(struct cell * game, int dim, int * dim_sizes, int * coords);

