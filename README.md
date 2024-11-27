#### Copyright 2024 Stoica Mihai-Bogdan (bogdanstoicasn@yahoo.com)

# Parallel Inverted Index Calculation Using Map-Reduce

## Overview

This project is a simple implementation of a parallel inverted index
calculation using the Map-Reduce programming model. The project is
written in C/C++ and uses the ```pthreads``` library for parallelism.

## Table of Contents

1. [Overview](#overview)
2. [Table of Contents](#table-of-contents)
3. [Introduction](#introduction)
4. [Running the Program](#running-the-program)
5. [Initialisation](#initialisation)
6. [Map Phase](#map-phase)
7. [Reduce Phase](#reduce-phase)
8. [Sorting Phase](#sorting-phase)
9. [Final Phase](#final-phase)
10. [Resources](#resources)
11. [Acknowledgements](#acknowledgements)

## Introduction

The Map-Reduce programming model is used for efficient parallel processing
of large datasets. The model consists of two main phases: the Map phase
and the Reduce phase. In the Map phase, the input data is divided into
key-value pairs. The Map function processes each key-value pair and
generates intermediate key-value pairs. In the Reduce phase, the intermediate
key-value pairs are grouped by key and processed to generate the final
output.

## Running the Program

### Method 1: Using Docker

Run the `run_with_docker.sh` script. This will build the Docker image and execute the program inside a Docker container.

### Method 2: Without Docker

Run the following commands:

```bash
cd ./checker
./checker.sh
```

### Additional Requirements

Environment variable command: `export LC_NUMERIC="en_US.UTF-8"`

Utility: `bc`


## Initialisation

The number of mappers, reducers, and input file are given as command-line
arguments. The input file contains the number of files to be processed
and the names of the files.

> **check_command_line** - checks the command line arguments and initializes
the mappers, reducers and usable mutexes and barriers.

> **parse_files_names** - extracts the names of the files to be processed and
adds them to 2 queues: one for the mappers and one for the reducers.

## Map Phase

A mapper in **The Map-Phase** does the following:

1. Reads a file name from the mapper queue, locking the mutex so that only one
mapper processes a file at a time.

2. Opens the file and reads the content word by word, transforming each word
in lowercase and removing any non-alphanumeric characters.

3. Generates key-value pairs ``(word - file_id)` and adds them to a vector of
queues, where the index of the vector is the `file_id`.

By locking the mutex, we ensure that only one mapper processes a file at a time
The file queue logic dynamically distributes files to available mappers, ensuring efficient load balancing.

The final barrier ensures that all mappers have finished processing their files before the reduce phase begins.

## Reduce Phase

A reducer in **The Reduce-Phase** does the following:

1. Reads an index from the index queue, locking the mutex so that only one
reducer processes an index at a time.

2. Processes the key-value pairs at the specified index.

3. Locks the beginning 

4. Checks for word existence. If it exists in the vector, the next step is to
check the appearance of the `file_id`. If the `file_id` exists, ignore the
word. If the `file_id` does not exist, add the `file_id` to the vector.
If the word does not exist, add the word and the `file_id` to the vector.

5. Waits for all reducers to finish processing their indexes before moving to
the sorting phase which is done dinamically.

To ensure efficient and thread-safe access to the shared data structures, a
separate mutex is associated with each letter of the alphabet. This way, only
one reducer can access the data structure for a specific letter at a time:

- When a word is processed, the mutex associated with the first letter of the
word is locked.

- This approach allows for efficient locking, preventing race conditions while minimizing contention.

## Sorting Phase

The sorting phase follows the reduce phase and is performed in the reducer
thread.

A queue is used to hold the letters to be processed. Each reducer dynamically
processes a letter from the vector of data. After completing the sorting for
one letter, the reducer continues looking for work until the queue is empty. 

To ensure thread safety, only the queue extraction process is locked, allowing
reducers to work independently on their assigned letters.

## Final Phase

In the final phase, a file is created for each letter of the alphabet,
containing the corresponding words in the format:
`word: [file_id1 file_id2 ...]`

This phase runs sequentially in the main thread after all worker threads have
joined. This choice avoids potential issues with parallel file writing, which
can lead to memory corruption and unnecessary overhead on typical systems.

## Resources

- [Map-Reduce Programming Model](https://en.wikipedia.org/wiki/MapReduce)
- [Apache Hadoop](https://hadoop.apache.org/docs/stable)
- [POSIX Threads Documentation](https://man7.org/linux/man-pages/man7/pthreads.7.html)

## Acknowledgements

This project was developed as part of the course "Parallel and Distributed
Algorithms" at the National University of Science and Technology POLITEHNICA
Bucharest, Faculty of Automatic Control and Computers.
I would like to thank Radu-Ioan Ciobanu and Alexandra Ana-Maria Ion for
their well explained and documented assignment. 










