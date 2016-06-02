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
void world_step(World* world, SDL_Point* toAdd, SDL_Point* toRemove);
int world_neighbor_count(World* world, int row, int col);
void world_print(World* world, FILE* file);
void world_render(World* world, SDL_Renderer* renderer, SDL_Point* offset, RenderConfig* config);

int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO);

    const int SCREEN_WIDTH = 1280;
    const int SCREEN_HEIGHT = 720;
    const int CELL_WIDTH = 8;
    const int CELL_HEIGHT = 8;
    const int CELL_COUNT = 10000;

    SDL_Window* window = SDL_CreateWindow("life", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1,  SDL_RENDERER_ACCELERATED);

    World world;
    int worldWidth = SCREEN_WIDTH, worldHeight = SCREEN_HEIGHT;
    world_init(&world, worldWidth, worldHeight);

    MTGen random;
    int seed = time(NULL);
    mt_seed(&random, seed);
    srand(seed);

    for (int i = 0; i < CELL_COUNT; ++i) {
        int32_t i = mt_next(&random, worldWidth * worldHeight);

        int r = i / worldWidth;
        int c = i % worldHeight;
        
        world_add(&world, r, c);
    }

    RenderConfig config;
    config.cellWidth = CELL_WIDTH;
    config.cellHeight = CELL_HEIGHT;

    SDL_Point cameraOffset = { 0, 0 };

    bool isRunning = true;
    bool shouldStep = false;

    int inputAxisX = 0;
    int inputAxisY = 0;

    float moveSpeed = 10.f;

    const int SCRATCH_SIZE = 4096 * 4096;
    SDL_Point* toAdd = calloc(SCRATCH_SIZE, sizeof(SDL_Point));
    SDL_Point* toRemove = calloc(SCRATCH_SIZE, sizeof(SDL_Point));

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

                    if (event.key.repeat == false) {
                        if (event.key.keysym.scancode == SDL_SCANCODE_UP) inputAxisY--;
                        else if (event.key.keysym.scancode == SDL_SCANCODE_DOWN) inputAxisY++;
                        else if (event.key.keysym.scancode == SDL_SCANCODE_LEFT) inputAxisX--;
                        else if (event.key.keysym.scancode == SDL_SCANCODE_RIGHT) inputAxisX++;
                    }
                    break;

                case SDL_KEYUP:
                    if (event.key.keysym.scancode == SDL_SCANCODE_UP) inputAxisY++;
                    else if (event.key.keysym.scancode == SDL_SCANCODE_DOWN) inputAxisY--;
                    else if (event.key.keysym.scancode == SDL_SCANCODE_LEFT) inputAxisX++;
                    else if (event.key.keysym.scancode == SDL_SCANCODE_RIGHT) inputAxisX--;
            }
        }

        if (shouldStep) {
            world_step(&world, toAdd, toRemove);
        }

        cameraOffset.x += inputAxisX * moveSpeed;
        cameraOffset.y += inputAxisY * moveSpeed;

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        world_render(&world, renderer, &cameraOffset, &config);

        SDL_RenderPresent(renderer);
    }

    free(toRemove);
    free(toAdd);

    SDL_DestroyRenderer(renderer);
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

typedef struct {
    World* world;
    int start;
    int end;
    SDL_Point* toAdd;
    SDL_Point* toRemove;
    int addCount;
    int removeCount;
} StepWorkerData;

int world_step_worker(void* ptr) {
    StepWorkerData* data = (StepWorkerData*)ptr;

    World* world = data->world;

    for (int i = data->start; i < data->end; ++i) {
        int row = i / world->width;
        int col = i % world->height;
        bool isDead = !world_has(world, row, col);
        int ncount = world_neighbor_count(world, row, col);
        if (isDead) {
            if (ncount == 3) {
                data->toAdd[data->addCount].x = row;
                data->toAdd[data->addCount].y = col;
                data->addCount++;
            }
        } else {
            if (ncount < 2 || ncount > 3) {
                data->toRemove[data->removeCount].x = row;
                data->toRemove[data->removeCount].y = col;
                data->removeCount++;
            }
        }
    }

    return 0;
}

void world_step(World* world, SDL_Point* toAdd, SDL_Point* toRemove) {
    int size = world->width * world->height;
    const int workerCount = 10;
    int cellsPerThread = size / workerCount;

    StepWorkerData* workerData = calloc(workerCount, sizeof(StepWorkerData));
    SDL_Thread** threads = calloc(workerCount, sizeof(SDL_Thread*));

    for (int i = 0; i < workerCount; ++i) {
        StepWorkerData* data = &workerData[i];

        data->world = world;
        data->start = i * cellsPerThread;
        data->end = (i + 1) * cellsPerThread;
        data->toAdd = &toAdd[data->start];
        data->toRemove = &toRemove[data->end];
        data->addCount = 0;
        data->removeCount = 0;
    }

    for (int i = 0; i < workerCount; ++i) {
        threads[i] = SDL_CreateThread(world_step_worker, "Step Worker", &workerData[i]);
    }

    for (int i = 0; i < workerCount; ++i) {
        int retVal = 0;
        SDL_WaitThread(threads[i], &retVal);
    }

    for (int i = 0; i < workerCount; ++i) {
        StepWorkerData* data = &workerData[i];

        for (int j = 0; j < data->addCount; ++j) {
            world_add(world,data->toAdd[j].x, data->toAdd[j].y);
        }

        for (int j = 0; j < data->removeCount; ++j) {
            world_remove(world,data->toRemove[j].x, data->toRemove[j].y);
        }
    }

    /*
    for (int row = 0; row < world->height; ++row) {
        for (int col = 0; col < world->width; ++col) {
            bool isDead = !world_has(world, row, col);
            int ncount = world_neighbor_count(world, row, col);
            if (isDead) {
                if (ncount == 3) {
                    toAdd[addIndex].x = row;
                    toAdd[addIndex].y = col;
                    addIndex++;
                }
            } else {
                if (ncount < 2 || ncount > 3) {
                    toRemove[removeIndex].x = row;
                    toRemove[removeIndex].y = col;
                    removeIndex++;
                }
            }
        }
    }

    for (int i = 0; i < addIndex; ++i) {
        world_add(world, toAdd[i].x, toAdd[i].y);
    }

    for (int i = 0; i < removeIndex; ++i) {
        world_remove(world, toRemove[i].x, toRemove[i].y);
    }
    */
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

void world_render(World* world, SDL_Renderer* renderer, SDL_Point* offset, RenderConfig* config) {
    int cellWidth = 32;
    int cellHeight = 32;

    if (config != NULL) {
        cellWidth = config->cellWidth;
        cellHeight = config->cellHeight;
    }

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_Rect bgRect = { -offset->x, -offset->y, world->width * cellWidth, world->height * cellHeight };
    SDL_RenderFillRect(renderer, &bgRect);

    SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
    SDL_Rect cellRect = { 0, 0, cellWidth, cellHeight };

    for (int row = 0; row < world->height; ++row) {
        for (int col = 0; col < world->width; ++col) {
            if (world_has(world, row, col)) {
                cellRect.x = col * cellWidth - offset->x;
                cellRect.y = row * cellHeight - offset->y;
                SDL_RenderFillRect(renderer, &cellRect);
            }
        }
    }
}
