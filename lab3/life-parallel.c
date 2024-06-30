#include "life.h"
#include <pthread.h>
#include <semaphore.h>

typedef struct {
    LifeBoard *state;
    LifeBoard *next_state;
    int start_row;
    int end_row;
    int steps;
    int threads;
    pthread_mutex_t *mutex;
    pthread_cond_t *cond;
    int *finished_thread;
} LifeTask;


void *update_state(void *arg) {
    LifeTask *task = (LifeTask *)arg;
    LifeBoard *state = task->state;
    LifeBoard *next_state = task->next_state;
    int steps = task->steps;
    int threads = task->threads;

    for(int step = 0; step < steps; step++){
        for (int y = task->start_row; y <= task->end_row; y+=threads) {
            for (int x = 1; x < state->width - 1; x++) {
                int live_neighbors = count_live_neighbors(state, x, y);
                LifeCell current_cell = at(state, x, y);
                LifeCell new_cell = (live_neighbors == 3) || (live_neighbors == 4 && current_cell == 1) ? 1 : 0;
                set_at(next_state, x, y, new_cell);
            }
        }
        pthread_mutex_lock(task->mutex);
        (*task->finished_thread)++;
        if ((*task->finished_thread) == threads) {
            (*task->finished_thread) = 0;
            swap(task->state, task->next_state);
            pthread_cond_broadcast(task->cond);
        } else {
            pthread_cond_wait(task->cond, task->mutex);
        }
        pthread_mutex_unlock(task->mutex);
    }

    return NULL;

}



void simulate_life_parallel(int threads, LifeBoard *state, int steps) {
    if (steps == 0) return;



    LifeBoard *next_state = create_life_board(state->width, state->height);
    if (next_state == NULL) {
        fprintf(stderr, "Failed to allocate memory for next state.\n");
        return;
    }

    pthread_mutex_t mutex;
    pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
    int finished_thread = 0;

    pthread_mutex_init(&mutex, NULL);

    pthread_t tid[threads];
    LifeTask tasks[threads];
    
    int rows_per_thread = (state->height - 2) / (threads + 1);
    int remaining_rows = (state->height - 2) % (threads + 1);


    // 创建并启动线程
    for (int i = 0; i < threads; i++){
        tasks[i].start_row = i + 1;
        tasks[i].end_row = (threads + 1) * (rows_per_thread - 1) + i + 1 + (i < remaining_rows ? (threads + 1): 0);
        tasks[i].state = state;
        tasks[i].next_state = next_state;
        tasks[i].steps = steps;
        tasks[i].threads = threads + 1;
        tasks[i].mutex = &mutex;
        tasks[i].cond = &cond;
        tasks[i].finished_thread = &finished_thread;
        pthread_create(&tid[i], NULL, update_state, &tasks[i]);

    }

    int main_end_row = (threads + 1) * rows_per_thread;
    for(int step = 0; step < steps; step++) {
        for (int y = threads + 1; y <= main_end_row; y+=(threads + 1)) {
            for (int x = 1; x < state->width - 1; x++) {
                int live_neighbors = count_live_neighbors(state, x, y);
                LifeCell current_cell = at(state, x, y);
                LifeCell new_cell = (live_neighbors == 3) || (live_neighbors == 4 && current_cell == 1) ? 1 : 0;
                set_at(next_state, x, y, new_cell);
            }
        }

        pthread_mutex_lock(&mutex);
        finished_thread ++ ;
        if (finished_thread == (threads + 1)) {
            finished_thread = 0;
            swap(state, next_state);
            pthread_cond_broadcast(&cond);
        } else {
            pthread_cond_wait(&cond, &mutex);
        }
        pthread_mutex_unlock(&mutex);
        
    }


    // 等待线程结束
    for (int i = 0; i < threads; i++) {
        pthread_join(tid[i], NULL);
    }

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);

    destroy_life_board(next_state);
}