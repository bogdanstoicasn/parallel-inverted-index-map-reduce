#include "tema1.h"

void check_command_line(int argc, char **argv, mapper *map, reducer *reducer)
{
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <mapper> <reducer> <input_file>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // mapper section
    map->mapper_num = atoi(argv[1]);
    printf("Number of mappers: %d\n", map->mapper_num);

    map->threads = (pthread_t *)malloc(map->mapper_num * sizeof(pthread_t));
    map->ids = (int *)malloc(map->mapper_num * sizeof(int));

    map->current_file_id = 0;

    pthread_mutex_init(&map->mut, NULL);

    // reducer section
    reducer->reducer_num = atoi(argv[2]);
    printf("Number of reducers: %d\n", reducer->reducer_num);

    reducer->threads = (pthread_t *)malloc(reducer->reducer_num * sizeof(pthread_t));
    reducer->ids = (int *)malloc(reducer->reducer_num * sizeof(int));
}

void parse_files_names(mapper *map, char *input_file)
{
    std::string input_file_str = "../checker/";
    input_file_str += input_file;
    FILE *file = fopen(input_file_str.c_str(), "r");
    if (file == NULL) {
        fprintf(stderr, "Error: Could not open file %s\n", input_file_str.c_str());
        exit(EXIT_FAILURE);
    }

    int number_of_files;
    fscanf(file, "%d", &number_of_files);

    // initialize the map->words vector with the correct number of files
    map->words.resize(number_of_files);

    // read the filenames and add them to the queue
    for (int i = 0; i < number_of_files; i++) {
        char file_name[100];
        fscanf(file, "%s", file_name);
        map->files.push(file_name);
    }

    fclose(file);
}


void *mapper_function(void *arg)
{
    mapper *map = (mapper *)arg;
    std::string input_file_base = "../checker/";

    while (1) {
        pthread_mutex_lock(&map->mut);
        if (map->files.empty()) {
            pthread_mutex_unlock(&map->mut);
            break;
        }

        std::string file_name = map->files.front();
        map->files.pop();
        int file_id = map->current_file_id++;
        pthread_mutex_unlock(&map->mut);

        printf("Mapper processing file %s (File ID: %d), by thread %ld\n", file_name.c_str(), file_id, pthread_self());

        // Open the file
        FILE *file = fopen((input_file_base + file_name).c_str(), "r");
        if (file == NULL) {
            fprintf(stderr, "Error: Could not open file %s\n", file_name.c_str());
            exit(EXIT_FAILURE);
        }

        // Read the words from the file
        while (1) {
            char word[100];
            if (fscanf(file, "%s", word) == EOF) {
                break;
            }

            // Add the word to the list of words
            //pthread_mutex_lock(&map->mut);

            // Convert the word to lowercase and clean it
            for (int i = 0; word[i]; i++) {
                word[i] = tolower(word[i]);
            }
            for (int i = 0; word[i]; i++) {
                if (!isalpha(word[i])) {
                    word[i] = '\0';
                    break;
                }
            }

            map->words[file_id].push_back(std::make_pair(std::string(word), file_id));
            //pthread_mutex_unlock(&map->mut);
        }

        fclose(file);
    }

    return NULL;
}

void *reducer_function(void *arg)
{
    return NULL;
}


void free_memory(mapper *map, reducer *red)
{
    // mapper section
    free(map->threads);
    free(map->ids);
    pthread_mutex_destroy(&map->mut);

    // reducer section
    free(red->threads);
    free(red->ids);
}

int main(int argc, char **argv)
{
    mapper map;
    reducer red;
    check_command_line(argc, argv, &map, &red);

    parse_files_names(&map, argv[3]);

    for (int i = 0; i < map.mapper_num; i++) {
        map.ids[i] = i;
        pthread_create(&map.threads[i], NULL, mapper_function, &map);
    }

    for (int i = 0; i < map.mapper_num; i++) {
        pthread_join(map.threads[i], &map.status);
    }

    // print the words
    for (long unsigned int i = 0; i < map.words.size(); i++) {
        printf("File ID: %ld\n", i);
        for (long unsigned int j = 0; j < map.words[i].size(); j++) {
            printf("%s %d\n", map.words[i][j].first.c_str(), map.words[i][j].second);
        }
    }

    free_memory(&map, &red);

    return 0;
}