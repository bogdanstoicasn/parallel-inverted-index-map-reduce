#ifndef TEMA1_H
#define TEMA1_H

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#include <vector>
#include <algorithm>
#include <string>
#include <queue>

typedef struct mapper {
    int mapper_num;

    pthread_t *threads;
    int *ids;

    // queue of files to be processed
    std::queue<std::string> files;
    int current_file_id;

    // every mapper has a list of {word, file_id} where file_id is the index of the file in the queue
    std::vector<std::queue<std::pair<std::string, int>>> words;

} mapper;

typedef struct reducer {
    int reducer_num;

    pthread_t *threads;
    int *ids;
    int current_file_id; // used to keep track of what vector to reduce

    // the reducers have only a list of {word, file_id1, file_id2, ...}
    // index 0 is letter a, index 1 is letter b, etc
    std::vector<std::vector<std::pair<std::string, std::vector<int>>>> words;


} reducer;

typedef struct wrapper {
    mapper *p_map;
    reducer *p_red;

    pthread_barrier_t bar;
    pthread_mutex_t mut;
    void *status;


} wrapper;

#endif