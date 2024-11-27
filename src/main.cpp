#include "tema1.h"

/*
    * Check the command line arguments and do basic initialization
    * @param argc - the number of arguments
    * @param argv - the arguments
    * @param wrap - the wrapper structure
 */
void check_command_line(int argc, char **argv, wrapper *wrap)
{
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <mapper> <reducer> <input_file>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if (atoi(argv[1]) < 1 || atoi(argv[2]) < 1) {
        fprintf(stderr, "Error: The number of mappers and reducers must be positive\n");
        exit(EXIT_FAILURE);
    }

    // mapper section
    wrap->p_map->mapper_num = atoi(argv[1]);

    wrap->p_map->threads = (pthread_t *)malloc(wrap->p_map->mapper_num * sizeof(pthread_t));
    wrap->p_map->ids = (int *)malloc(wrap->p_map->mapper_num * sizeof(int));

    wrap->p_map->current_file_id = 0;

    // reducer section
    wrap->p_red->reducer_num = atoi(argv[2]);

    wrap->p_red->threads = (pthread_t *)malloc(wrap->p_red->reducer_num * sizeof(pthread_t));
    wrap->p_red->ids = (int *)malloc(wrap->p_red->reducer_num * sizeof(int));

    // mutex and barrier section
    pthread_mutex_init(&wrap->mut, NULL);
    for (int i = 0; i < 26; i++) {
        pthread_mutex_init(&wrap->mut_words[i], NULL);
    }
    pthread_barrier_init(&wrap->bar, NULL, wrap->p_map->mapper_num + wrap->p_red->reducer_num);
    pthread_barrier_init(&wrap->bar_sort, NULL, wrap->p_red->reducer_num);
}

/*
    * Parse the input file and put the filenames in the queue
    * @param wrap - the wrapper structure
    * @param input_file - the input file
 */
void parse_files_names(wrapper *wrap, char *input_file)
{
    std::string input_file_str = "../checker/";
    input_file_str += input_file;
    FILE *file = fopen(input_file_str.c_str(), "r");
    if (!file) {
        fprintf(stderr, "Error: Could not open file %s\n", input_file_str.c_str());
        exit(EXIT_FAILURE);
    }

    int number_of_files;
    fscanf(file, "%d", &number_of_files);

    // initialize the map->words vector with the correct number of files
    wrap->p_map->words.resize(number_of_files);
    
    // put the index of the files in the queue
    for (int i = 0; i < number_of_files; i++) {
        wrap->p_red->files.push(i);
    }

    // push the letters in the queue
    for (int i = 0; i < 26; i++) {
        wrap->p_red->letters.push(std::string(1, (char)('a' + i)));
    }

    // read the filenames and add them to the queue
    for (int i = 0; i < number_of_files; i++) {
        char file_name[100];
        fscanf(file, "%s", file_name);
        wrap->p_map->files.push(std::string(file_name));
    }

    fclose(file);
}

/*
    * The function that the mapper threads will execute
    * @param arg - the wrapper structure
 */
void *mapper_function(void *arg)
{
    wrapper *wrap = (wrapper *)arg;
    std::string input_file_base = "../checker/";

    while (1) {
        // get the next file to be processed
        pthread_mutex_lock(&wrap->mut);
        if (wrap->p_map->files.empty()) {
            pthread_mutex_unlock(&wrap->mut);
            break;
        }

        std::string file_name = wrap->p_map->files.front();
        wrap->p_map->files.pop();
        int file_id = wrap->p_map->current_file_id++;
        pthread_mutex_unlock(&wrap->mut);

        FILE *file = fopen((input_file_base + file_name).c_str(), "r");
        if (!file) {
            fprintf(stderr, "Error: Could not open file %s\n", file_name.c_str());
            exit(EXIT_FAILURE);
        }

        // read the words from the file
        while (1) {
            char word[100];
            if (fscanf(file, "%s", word) == EOF) {
                break;
            }

            // convert the word to lowercase
            for (int i = 0; word[i]; i++) {
                word[i] = tolower(word[i]);
            }
            
            // now ignore the special characters, but if we have word1-word2 we consider it as word1word2
            for (int i = 0; word[i]; i++) {
                if (!isalpha(word[i])) {
                    for (int j = i; word[j]; j++) {
                        word[j] = word[j + 1];
                    }
                    i--;
                }
            }
            
            // don't need lock here because we have a different queue for each file
            wrap->p_map->words[file_id].push(std::make_pair(std::string(word), file_id));
        }

        fclose(file);
    }

    // wait till all mappers finish
    pthread_barrier_wait(&wrap->bar);

    return NULL;
}

/*
    * The function that the reducer threads will execute
    * @param arg - the wrapper structure
 */
void *reducer_function(void *arg)
{
    // wait until all mappers finish
    pthread_barrier_wait(&((wrapper *)arg)->bar);

    wrapper *wrap = (wrapper *)arg;

    while (1) {
        // get the next file to be processed(basically the index of the queue we have to process)
        pthread_mutex_lock(&wrap->mut);
        if (wrap->p_red->files.empty()) {
            pthread_mutex_unlock(&wrap->mut);
            break;
        }

        int file_id = wrap->p_red->files.front();
        wrap->p_red->files.pop();
        pthread_mutex_unlock(&wrap->mut);

        // process the words from the queue
        while (!wrap->p_map->words[file_id].empty()) {
            std::pair<std::string, int> word = wrap->p_map->words[file_id].front();
            wrap->p_map->words[file_id].pop();
            if (word.first.empty()) {
                continue;
            }

            // lock just a letter at a time so we dont have to lock the whole vector
            int index = word.first[0] - 'a';
            pthread_mutex_lock(&wrap->mut_words[index]);

            bool found = false;
            for (long unsigned int i = 0; i < wrap->p_red->words[index].size(); i++) {
                if (wrap->p_red->words[index][i].first == word.first) {
                    found = true;
                    // if index is not in the vector, add it
                    if (std::find(wrap->p_red->words[index][i].second.begin(), wrap->p_red->words[index][i].second.end(), word.second) == wrap->p_red->words[index][i].second.end()) {
                        wrap->p_red->words[index][i].second.push_back(word.second);

                    }
                    break;
                }
            }

            // if the word is not in the vector, add it
            if (!found) {
                wrap->p_red->words[index].push_back(std::make_pair(word.first, std::vector<int>()));
                wrap->p_red->words[index][wrap->p_red->words[index].size() - 1].second.push_back(word.second);
            }
            
            pthread_mutex_unlock(&wrap->mut_words[index]);
        } 
    }

    // wait till all reducers finish
    pthread_barrier_wait(&wrap->bar_sort);

    // sorting the words section(it's done dynamically)
    while (1) {
        pthread_mutex_lock(&wrap->mut);
        if (wrap->p_red->letters.empty()) {
            pthread_mutex_unlock(&wrap->mut);
            break;
        }

        std::string letter = wrap->p_red->letters.front();
        wrap->p_red->letters.pop();
        pthread_mutex_unlock(&wrap->mut);

        int index = letter[0] - 'a';

        // sort the words by descending order of the number of files they appear in
        // and also alphabetically if they appear in the same number of files
        std::sort(wrap->p_red->words[index].begin(), wrap->p_red->words[index].end(), 
            [](const std::pair<std::string, std::vector<int>> &a, const std::pair<std::string, std::vector<int>> &b) {
                if (a.second.size() == b.second.size()) {
                    return a.first < b.first;
                }
                return a.second.size() > b.second.size();
            });
        
        // sort the file indexes ascending
        for (long unsigned int i = 0; i < wrap->p_red->words[index].size(); i++) {
            std::sort(wrap->p_red->words[index][i].second.begin(), wrap->p_red->words[index][i].second.end());
        }
    }
     
    return NULL;
}

/*
    * Free the memory allocated
    * Destroy the mutexes and the barriers
    * @param wrap - the wrapper structure
*/
void free_memory(wrapper *wrap)
{
    // mapper section
    free(wrap->p_map->threads);
    free(wrap->p_map->ids);

    // reducer section
    free(wrap->p_red->threads);
    free(wrap->p_red->ids);

    // mutex and barrier section
    pthread_mutex_destroy(&wrap->mut);
    for (int i = 0; i < 26; i++) {
        pthread_mutex_destroy(&wrap->mut_words[i]);
    }
    pthread_barrier_destroy(&wrap->bar);
    pthread_barrier_destroy(&wrap->bar_sort);
}

int main(int argc, char **argv)
{

    wrapper wrap; mapper map; reducer red;
    wrap.p_map = &map;
    wrap.p_red = &red;

    check_command_line(argc, argv, &wrap);

    parse_files_names(&wrap, argv[3]);

    // create the threads
    for (int i = 0; i < map.mapper_num + red.reducer_num; i++) {
        if (i < map.mapper_num) {
            map.ids[i] = i;
            pthread_create(&map.threads[i], NULL, mapper_function, &wrap);
        } else {
            red.ids[i - map.mapper_num] = i;
            pthread_create(&red.threads[i - map.mapper_num], NULL, reducer_function, &wrap);
        }
    }

    // wait for the threads to finish
    for (int i = 0; i < map.mapper_num + red.reducer_num; i++) {
        if (i < map.mapper_num) {
            pthread_join(map.threads[i], &wrap.status);
        } else {
            pthread_join(red.threads[i - map.mapper_num], &wrap.status);
        }
    }

    // write in the files a.txt, b.txt, etc
    for (long unsigned int i = 0; i < 26; i++) {
        std::string file_name = "";
        file_name += (char)('a' + i);
        file_name += ".txt";
        FILE *file = fopen(file_name.c_str(), "w");
        if (!file) {
            fprintf(stderr, "Error: Could not open file %s\n", file_name.c_str());
            exit(EXIT_FAILURE);
        }

        // write the words in the file
        for (long unsigned int j = 0; j < red.words[i].size(); j++) {
            fprintf(file, "%s:[", red.words[i][j].first.c_str());
            for (long unsigned int k = 0; k < red.words[i][j].second.size(); k++) {
                fprintf(file, "%d", red.words[i][j].second[k] + 1);
                if (k != red.words[i][j].second.size() - 1) {
                    fprintf(file, " ");
                }
            }
            fprintf(file, "]\n");
        }
        fclose(file);
    }

    free_memory(&wrap);

    return 0;
}