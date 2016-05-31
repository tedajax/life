#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>
#include <memory.h>
#include <unistd.h>

typedef struct {
    int width;
    int height;
    int* cells;
    int cellCount;
} World;

int hash_cell(int row, int col);
void world_init(World* world, int width, int height);
void world_copy(World* src, World* dst);
void world_free(World* world);
void world_set_data(World* world, char* data);
void world_add_cell(World* world, int row, int col);
void world_remove_cell(World* world, int row, int col);
int world_index_of(World* world, int row, int col);
void world_step(World* world);
int world_neighbor_count(World* world, int row, int col);
void world_print(World* world, FILE* file);

int main(int argc, char* argv[]) {

    char* test = " *                  "
                 "  **                "
                 " **                 ";

    World world;
    world_init(&world, 20, 20);
    world_set_data(&world, test);

    while (true) {
        world_print(&world, stdout);
        printf("\n");
        world_step(&world);
        usleep(250000);
    }

    return 0;
}

int hash_cell(int row, int col) {
    int hash = 23;
    hash = hash * 31 + row;
    hash = hash * 31 + col;
    return hash;
} 

void world_init(World* world, int width, int height) {
    world->width = width;
    world->height = height;
    world->cells = calloc(width * height, sizeof(int));
    world->cellCount = 0;
}

void world_copy(World* src, World* dst) {
    dst->width = src->width;
    dst->height = src->height;
    int size = src->width * src->height;
    dst->cells = calloc(size, sizeof(int));
    memcpy(dst->cells, src->cells, size * sizeof(int)); 
    dst->cellCount = src->cellCount;
}

void world_free(World* world) {
    assert(world->cells != NULL);
    free(world->cells);
    world->cells = NULL;
}

void world_set_data(World* world, char* data) {
    int index = 0;
    int size = world->width * world->height;
    while (data[index] && world->cellCount < size) {
        if (!isspace(data[index])) {
            int row = index / world->width;
            int col = index % world->width;
            world_add_cell(world, row, col);
        }
        ++index;
    }
}

int world_index_of(World* world, int row, int col) {
    int hash = hash_cell(row, col);

    for (int i = 0; i < world->cellCount; ++i) {
        if (world->cells[i] == hash) {
            return i;
        }
    }

    return -1;
}

void world_add_cell(World* world, int row, int col) {
    if (world_index_of(world, row, col) >= 0) {
        return;
    }

    int hash = hash_cell(row, col);
    world->cells[world->cellCount] = hash;
    world->cellCount++;
}

void world_remove_cell(World* world, int row, int col) {
    int index = world_index_of(world, row, col);

    if (index < 0) {
        return;
    }

    for (int i = index; i < world->cellCount - 1; ++i) {
        world->cells[i] = world->cells[i + 1];
    }

    world->cellCount--;
}

int world_neighbor_count(World* world, int row, int col) {
    int count = 0;
    if (world_index_of(world, row - 1, col) >= 0) ++count;
    if (world_index_of(world, row + 1, col) >= 0) ++count;
    if (world_index_of(world, row, col - 1) >= 0) ++count;
    if (world_index_of(world, row, col + 1) >= 0) ++count;
    if (world_index_of(world, row - 1, col - 1) >= 0) ++count;
    if (world_index_of(world, row + 1, col - 1) >= 0) ++count;
    if (world_index_of(world, row - 1, col + 1) >= 0) ++count;
    if (world_index_of(world, row + 1, col + 1) >= 0) ++count;
    return count;
}


void world_step(World* world) {
    World old;
    world_copy(world, &old);

    for (int row = 0; row < world->height; ++row) {
        for (int col = 0; col < world->width; ++col) {
            bool isDead = (world_index_of(&old, row, col) < 0);
            int ncount = world_neighbor_count(&old, row, col);
            if (isDead) {
                if (ncount == 3) {
                    world_add_cell(world, row, col);
                }
            } else {
                if (ncount < 2 || ncount > 3) {
                    world_remove_cell(world, row, col);
                }
            }
        }
    }

    world_free(&old);
}

void world_print(World* world, FILE* file) {
    for (int col = 0; col < world->width + 2; ++col) { fprintf(file, "-"); }
    fprintf(file, "\n");

    for (int row = 0; row < world->height; ++row) {
        fprintf(file, "|");
        for (int col = 0; col < world->width; ++col) {
            char c = (world_index_of(world, row, col) >= 0) ? 'O' : '.';
            fprintf(file, "%c", c);
        }
        fprintf(file, "}\n");
    }
    
    for (int col = 0; col < world->width + 2; ++col) { fprintf(file, "-"); }
    fprintf(file, "\n");
}

