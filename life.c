#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>
#include <memory.h>
#include <time.h>

#include <SDL2/SDL.h>

#include "mtwist.h"

typedef struct {
    int hash;
    int row;
    int col;
} Cell;

typedef struct {
    Cell* data;
    int capacity;
    int count;
} CellBucket;

typedef struct {
    int width;
    int height;
    int count;
    CellBucket* buckets;
    int bucketCount;
} World;

typedef struct {
    int cellWidth;
    int cellHeight;
} RenderConfig;

int hash_cell(int row, int col);
void world_init(World* world, int width, int height);
void world_copy(World* src, World* dst);
void world_free(World* world);
void world_set_data(World* world, char* data);
bool world_has(World* world, int row, int col);
void world_add(World* world, int row, int col);
void world_remove(World* world, int row, int col);
void world_step(World* world);
int world_neighbor_count(World* world, int row, int col);
void world_print(World* world, FILE* file);
void world_render(World* world, SDL_Renderer* renderer, RenderConfig* config);

int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO);

    const int SCREEN_WIDTH = 1920;
    const int SCREEN_HEIGHT = 1080;
    const int CELL_WIDTH = 4;
    const int CELL_HEIGHT = 2;
    const int CELL_COUNT = 10000;

    SDL_Window* window = SDL_CreateWindow("life", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1,  SDL_RENDERER_ACCELERATED);

    World world;
    int worldWidth = SCREEN_WIDTH / CELL_WIDTH, worldHeight = SCREEN_HEIGHT / CELL_HEIGHT;
    world_init(&world, worldWidth, worldHeight);

    MTGen random;
    int seed = 0;
    mt_seed(&random, seed);
    srand(seed);

    for (int i = 0; i < argc; ++i) {
        printf("%02d) %s\n", i, argv[i]);
    }

    bool useMT = true;

    if (argc >= 2) {
        int ai = atoi(argv[1]);
        useMT = (ai > 0);
    }

    printf("Mersenne Twister On: %s\n", (useMT) ? "true" : "false");

    for (int i = 0; i < CELL_COUNT; ++i) {
        int r = 0, c = 0;
        if (useMT) {
            int32_t i = mt_next(&random, worldWidth * worldHeight);

            r = i / worldWidth;
            c = i % worldHeight;
        }
        else {
            r = rand() % worldHeight;
            c = rand() % worldWidth;
        }
        world_add(&world, r, c);
    }

    RenderConfig config;
    config.cellWidth = CELL_WIDTH;
    config.cellHeight = CELL_HEIGHT;

    bool isRunning = true;
    bool shouldStep = false;

    while (isRunning) {
        SDL_Event event;
        while (SDL_PollEvent(&event) != 0) {
            switch (event.type) {
                case SDL_QUIT:
                    isRunning = false;
                    break;

                case SDL_KEYDOWN:
                    if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
                        isRunning = false;
                    }
                    else if (event.key.keysym.scancode == SDL_SCANCODE_SPACE) {
                        shouldStep = !shouldStep;
                    }
                    break;
            }
        }

        if (shouldStep) {
            world_step(&world);
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        world_render(&world, renderer, &config);

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyWindow(window);

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
    world->count = 0;
    world->bucketCount = 1024;
    world->buckets = calloc(world->bucketCount, sizeof(CellBucket));
}

void world_copy(World* src, World* dst) {
    dst->width = src->width;
    dst->height = src->height;
    dst->bucketCount = src->bucketCount;
    dst->buckets = calloc(dst->bucketCount, sizeof(CellBucket));
    
    for (int i = 0; i < src->bucketCount; ++i) {
        CellBucket bucket = src->buckets[i];

        if (bucket.count > 0) {
            dst->buckets[i].data = calloc(bucket.capacity, sizeof(Cell));
            dst->buckets[i].count = bucket.count;
            dst->buckets[i].capacity = bucket.capacity;
            memcpy(dst->buckets[i].data, bucket.data, bucket.count * sizeof(Cell));
        }
    }

    dst->count = src->count;
}

void world_free(World* world) {
    for (int i = 0; i < world->bucketCount; ++i) {
        if (world->buckets[i].data != NULL) {
            free(world->buckets[i].data);
        }
    }
    free(world->buckets);
    world->buckets = NULL;
}

void world_set_data(World* world, char* data) {
    /*int index = 0;
    int size = world->width * world->height;
    while (data[index] && world->cellCount < size) {
        if (!isspace(data[index])) {
            int row = index / world->width;
            int col = index % world->width;
            world_add(world, row, col);
        }
        ++index;
    }*/
}

bool world_has(World* world, int row, int col) {
    int hash = hash_cell(row, col);
    int bucketIndex = hash % world->bucketCount;

    CellBucket bucket = world->buckets[bucketIndex];

    for (int i = 0; i < bucket.count; ++i) {
        if (bucket.data[i].hash == hash) {
            return true;
        }
    }

    return false;
}

void world_add(World* world, int row, int col) {
    if (world_has(world, row, col)) {
        return;
    }

    int hash = hash_cell(row, col);
    int bucketIndex = hash % world->bucketCount;

    CellBucket* bucket = &world->buckets[bucketIndex];

    if (bucket->capacity == 0) {
        bucket->capacity = 64;
        bucket->data = calloc(bucket->capacity, sizeof(Cell));
    }

    if (bucket->count >= bucket->capacity) {
        bucket->capacity *= 1.5;
        bucket->data = realloc(bucket->data, bucket->capacity * sizeof(Cell));
    }

    Cell cell = {
        .hash = hash,
        .row = row,
        .col = col,
    };

    bucket->data[bucket->count] = cell;
    bucket->count++;

    world->count++;
}

void world_remove(World* world, int row, int col) {
    int hash = hash_cell(row, col);
    int bucketIndex = hash % world->bucketCount;

    CellBucket* bucket = &world->buckets[bucketIndex];

    int index = -1;
    for (int i = 0; i < bucket->count; ++i) {
        if (bucket->data[i].hash == hash) {
            index = i;
            break;
        }    
    }

    if (index < 0) {
        return;
    }

    for (int i = index; i < bucket->count - 1; ++i) {
        bucket->data[i] = bucket->data[i + 1];
    }
    bucket->count--;

    world->count--;
}

int world_neighbor_count(World* world, int row, int col) {
    int count = 0;
    if (world_has(world, row - 1, col)) ++count;
    if (world_has(world, row + 1, col)) ++count;
    if (world_has(world, row, col - 1)) ++count;
    if (world_has(world, row, col + 1)) ++count;
    if (world_has(world, row - 1, col - 1)) ++count;
    if (world_has(world, row + 1, col - 1)) ++count;
    if (world_has(world, row - 1, col + 1)) ++count;
    if (world_has(world, row + 1, col + 1)) ++count;
    return count;
}


void world_step(World* world) {
    World old;
    world_copy(world, &old);

    for (int row = 0; row < world->height; ++row) {
        for (int col = 0; col < world->width; ++col) {
            bool isDead = !world_has(&old, row, col);
            int ncount = world_neighbor_count(&old, row, col);
            if (isDead) {
                if (ncount == 3) {
                    world_add(world, row, col);
                }
            } else {
                if (ncount < 2 || ncount > 3) {
                    world_remove(world, row, col);
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
            char c = (world_has(world, row, col)) ? 'O' : '.';
            fprintf(file, "%c", c);
        }
        fprintf(file, "}\n");
    }
    
    for (int col = 0; col < world->width + 2; ++col) { fprintf(file, "-"); }
    fprintf(file, "\n");
}

void world_render(World* world, SDL_Renderer* renderer, RenderConfig* config) {
    int cellWidth = 32;
    int cellHeight = 32;

    if (config != NULL) {
        cellWidth = config->cellWidth;
        cellHeight = config->cellHeight;
    }

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_Rect bgRect = { 0, 0, world->width * cellWidth, world->height * cellHeight };
    SDL_RenderFillRect(renderer, &bgRect);

    SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
    SDL_Rect cellRect = { 0, 0, cellWidth, cellHeight };

    for (int row = 0; row < world->height; ++row) {
        for (int col = 0; col < world->width; ++col) {
            if (world_has(world, row, col)) {
                cellRect.x = col * cellWidth;
                cellRect.y = row * cellHeight;
                SDL_RenderFillRect(renderer, &cellRect);
            }
        }
    }
}
