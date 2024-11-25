#ifndef TEMA1_H
#define TEMA1_H

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string>
#include <queue>

typedef struct mapper {
    int mapper_num;
    void *status;

    pthread_t *threads;
    int *ids;
    pthread_mutex_t mut;

    // queue of files to be processed
    std::queue<std::string> files;
    int current_file_id;

    // every mapper has a list of {word, file_id} where file_id is the index of the file in the queue
    std::vector<std::vector<std::pair<std::string, int>>> words;

} mapper;

typedef struct reducer {
    int reducer_num;
    void *status;

    pthread_t *threads;
    int *ids;
    pthread_mutex_t mut;

    // the reducers have only a list of {word, file_id1, file_id2, ...}
    // index 0 is letter a, index 1 is letter b, etc
    std::vector<std::vector<std::pair<std::string, std::vector<int>>>> words;


} reducer;

#endif