# Parallel-Game-of-Life | CIS*3090 Assignment 
# Author: Cameron Norrie | Date: Oct 13, 2022

This program is a parallelized version of Conway's Game of Life using Pthreads in C. Implements thread-based concurrency 
to distribute workload across multiple segments of the game board, achieving efficient parallel execution. It Demonstrates proficiency 
in Linux system programming, threading models, and problem-solving skills in optimizing performance for parallel computation.


# Functionality:
This parallel implementation of Conway's Game of Life leverages Pthreads to achieve efficient parallel execution. 
It simulates the classic cellular automaton on a 2D grid, adhering to Conway's rules for cell evolution.
The program divides the grid into segments and assigns each segment to a separate thread, enabling concurrent processing and maximizing computational efficiency.
Additionally, the program employs pthread barriers for synchronization, ensuring consistent state transitions across threads and facilitating accurate simulation results.

# Usage
gol_data nThreads gridSize nIterations -d

where:
- nThreads is the number of threads the program will create to run the game. This is a
required parameter.
- gridSize is one number that represents both the height and width of the grid. This is a required parameter.
- nIterations is how many iterations of the game the program will run before exiting. This is a required parameter.
- -d indicates that the program should print out the grid after each iteration. This is an optional argument:
- If the -d is present then the output should be displayed
- If the -d is not present then the output is not displayed. Each new grid is calculated but
the grid is never drawn on the screen. This is an optional parameter.

# Compilation
$ make

# Execution
./gol_threads 2 100 5 -d
