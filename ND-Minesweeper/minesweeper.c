#include "minesweeper.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

/*
  Count the total number of cells needed to store in the game array
*/
int count_cells(int dim, int * dim_sizes) {
    int total_cells = 1;
    for (int i = 0; i < dim; i++) {
        total_cells = total_cells * dim_sizes[i];
    }
    return total_cells;
}

/*
  Calculate the offset of 1-dimensional game array,
  based on the given multi-dimentional coordinates
*/
int calculate_offset(int dim, int * dim_sizes, int * coords) {
    int cursor = 0;
    int muptiple = 1;
    for (int d = dim - 1; d >= 0; d--) { // index of dim_sizes
        muptiple = 1;
        for (int i = dim - d; i <= dim - 1; i++) {
            muptiple = muptiple * dim_sizes[i];
        }
        cursor = cursor + coords[dim - 1 - d] * muptiple;
    }
    return cursor;
}


/*
  Create and insert cell to game array,
  based om the given coordinates and mined or not
*/
void init_cells(struct cell * game, int * dim_sizes, int * coords,
  int mined, int dim) {

    // Initialize empty new cell on heap
    struct cell * new_cell;

    // Calculate position for the multi-dimentional game array
    int cursor = calculate_offset( dim, dim_sizes, coords);
    new_cell = game + cursor;

    // Add value
    new_cell->mined = mined;
    new_cell->selected = 0;
    for (int i = 0; i < dim; i++) {
        new_cell->coords[i] = coords[i];
    }
}


/*
  Check if the given coordinate is a mined cell
  Returns 1 if the given coordinate is a mined cell
  If not mined cell, return 0
*/
int check_mined(int * coor, int ** mined_cells, int dim, int num_mines) {

    int count = 0;
    for (int i = 0; i < num_mines; i++) {
        count = 0;
        for (int d = 0; d < dim; d++) {
            if (mined_cells[i][d] == coor[d]) {
                count++;
            }
        }
        if (count == dim){ // the given coor is a mined cell
            return 1;
        }
    }
    return 0;
}

/*
  Helper function for init_game
  Search and collect cells adjacent the given cell, based on the coordinates
*/
void search_adjacent(struct cell * game, int index, int * coor,
  int dim, int total_cells) {

    struct cell * adjacent[MAX_ADJACENT];

    int count = 0; // number of adjacent
    int itself = 1; // itself = 1 means current cell is itself

    // close = 1 means the coordinte difference between 2 cells is less than 1
    int close = 1;

    for (int i = 0; i < total_cells; i ++){
        close = 1;
        itself = 1;
        for (int d = 0; d < dim; d++) {
            // adjacent should not be itself
            if (abs((game + i)->coords[d] - coor[d]) != 0) {
               itself = 0; // not itself
            }
            // adjacent's coor <= 1
            if (abs((game + i)->coords[d] - coor[d]) > 1) {
                close = 0;
            }
        }
        if (close == 1 && itself == 0) {  // exclude itself
            adjacent[count] = (game + i); // find an adjacent
            count ++;
        }
    }

    // assign adjacents to the game's cell
    for (int i = 0; i < count; i++) {
        (game + index)->adjacent[i] = adjacent[i];
    }
    (game + index)->num_adjacent = count; //exclude itself
    return;
}


/*
  Count num_adjacent for each cell, also collect its adjacents
*/
void count_adjacent(struct cell * game, int * dim_sizes, int dim) {

    // count the total number of cells needed to store inside
    int total_cells = count_cells(dim, dim_sizes);

    for (int j = 0; j < total_cells; j++) {
        search_adjacent(game, j, (game + j)->coords, dim, total_cells);
    }
}

/*
  The recursion stops when a bounary is reached and all all adjacent cells
  are already selected before.
  Returns 0 if at least one of its adjacent hasnot been visit.
  Otherwise, return 1, which means that all adjacent cells have been selected
*/
int check_all_adjacent_selected(struct cell * game, int * coords,
  int num_adjacent, struct cell * adjacent) {

    for (int i = 0; i < num_adjacent; i++) {
        // at least one of its adjacent hasnot been selected
        if ((adjacent + i )->selected == 0) {
            return 0;
        }
    }
    return 1; // all adjacent cells are already selected
}

/*
    Check if all cell except that contain mine have been selected
    Returns 1 means we win the game
*/
int check_win(struct cell * game, int * dim_sizes, int dim) {

    // count the total number of cells needed to store inside
    int total_cells = count_cells(dim, dim_sizes);

    for (int j = 0; j < total_cells; j++) {

        // find unmined cell that havenot been selected,
        if ((game + j)->selected != 1 && (game + j)->mined == 0)
            return 0; // not win
    }
    return 1;
}


/*
    Search the number of mined adjacent cell, based on the given coords
    Returns the number of mined adjacent cell
*/
int search_mined_adjacent(struct cell * game, int * coords, int num_adjacent,
  struct cell ** adjacent){
    int hint = 0;
    for (int i = 0; i < num_adjacent; i++) {
        if (adjacent[i]->mined == 1) {
            hint ++; // find a mined adjacent
        }
    }
    return hint;
}


/*
    Recursively automatically select adjacent cells
*/
void select_recursion(struct cell * game, int * coords, int dim,
  int * dim_sizes){

    // Find the position of current cell
    int cursor = calculate_offset(dim, dim_sizes, coords);

    // Calculate hint by searching the number of its mined adjacent cell
    (game + cursor)->hint = search_mined_adjacent(game, coords,
                                                (game + cursor)->num_adjacent,
                                                (game + cursor)->adjacent);

    // Base case 1: the select cell's adjacent has at least one mine
    if ((game + cursor)->hint > 0) {
        return;
    }

    // The select cell has 0 adjacent mines,
    // then we need recursively check its adjacent cells
    for (int k = 0; k < (game + cursor)->num_adjacent; k++) {

        if ((game + cursor)->adjacent[k]->selected != 1){ // havenot visit
            // mark cell as visited
            (game + cursor)->adjacent[k]->selected = 1;
            // get coordinates of adjacents
            int * adj_coords = (game + cursor)->adjacent[k]->coords;
            // continue do recursion on unvisited cells
            select_recursion(game, adj_coords, dim, dim_sizes);
        }
    }

    return;
}

/*
    Initialize mined cell, and insert it to game array, based on coords given
*/
void init_mined_cells(struct cell * game, int dim, int * dim_sizes,
  int * coords, int num_mines, int ** mined_cells) {
    for (int i = 0; i < num_mines; i++){
        for (int j = 0; j< dim; j++){
            *(coords+j) = mined_cells[i][j];
        }
        init_cells(game, dim_sizes, coords, 1, dim); // mine is 1
    }
}

/*
    Initialize unmined cells, and insert it to game array, based on coords given
*/
void init_unmined_cells(struct cell * game, int dim, int * dim_sizes,
  int * coords, int num_mines, int ** mined_cells) {

    // count the total number of cells needed to store inside
    int total_cells = count_cells(dim, dim_sizes);

    for (int cursor = 0; cursor < total_cells; cursor++){
        // collect the coords based on offset
        int offset = cursor;
        for (int d = dim - 1; d > 0; d--) {
            coords[d] = offset % dim_sizes[d];
            offset = offset / dim_sizes[d];
        }
        coords[0] = offset;

        // check if the cell based on given coords is mined
        int find = check_mined(coords, mined_cells, dim, num_mines);
        if (find == 0) // unmined cells
            init_cells(game, dim_sizes, coords, 0, dim); // mine is 0
    }
}


/*
    Initialise game array based on dimension, dimension size,
    number of minded cells, and coordinates of mined cells
*/
void init_game(struct cell * game, int dim, int * dim_sizes, int num_mines,
  int ** mined_cells) {

    // Initialize cells' coords
    int * coords = (int *)calloc(dim, sizeof(int));

    // Initialize mined cells
    init_mined_cells(game, dim, dim_sizes, coords, num_mines, mined_cells);

    // Initialise other cells
    init_unmined_cells(game, dim, dim_sizes, coords, num_mines, mined_cells);

    // Search and write adjacent for each cell
    count_adjacent(game, dim_sizes, dim);

    // Free memory
    free(coords);

    return;
}

/*
    Update game array's information for each move of the player,
    also check player wins or not.
*/
int select_cell(struct cell * game, int dim, int * dim_sizes, int * coords) {

    // Find the position of the selected cell
    int cursor = calculate_offset(dim, dim_sizes, coords);

    // If the player selects a cell that is out of bounds (invalid input),
    // Returns 0 and do nothing
    if (cursor > count_cells(dim, dim_sizes) || cursor < 0) {
        return 0;
    }
    (game + cursor)->selected = 1;

    // Case 1: the selected cell with a mine
    if ((game + cursor)->mined == 1) {
        return 1;
    }

    // Case 2: the selected cell without a mine
    if ((game + cursor)->mined == 0) {
        select_recursion(game, coords, dim, dim_sizes);
    }

    // Check if all cell except that contain mine have been selected
    int win = check_win(game, dim_sizes, dim);
    if (win == 1)
        return 2;
    return 0;
}
