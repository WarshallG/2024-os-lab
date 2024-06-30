#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "life.h"

#define NEED_PRINT 0

int main(int argc, char** argv){
    if(argc == 3){
        int steps = 0;
        char input_file[256];
        steps = atoi(argv[1]);
        strcpy(input_file, argv[2]);
        LifeBoard* board = malloc(sizeof(LifeBoard));
        FILE* in = fopen(input_file, "r");
        read_life_board(in, board);

        // struct timeval start_time, end_time;
        // gettimeofday(&start_time, NULL);
        simulate_life_serial(board, steps);

        // gettimeofday(&end_time, NULL);
        // long seconds = end_time.tv_sec - start_time.tv_sec;
        // long microseconds = end_time.tv_usec - start_time.tv_usec;
        // double elapsed_time = seconds * 1000 + microseconds / 1000.0;
        // printf("Use time: %.5fs\n", elapsed_time/1000);

        if(NEED_PRINT){
            print_life_board(board);
        }
        
        destroy_life_board(board);
        return 0;
    }else if(argc == 4){
        int steps = 0;
        int threads = 1;
        char input_file[256];
        threads = atoi(argv[1]);
        steps = atoi(argv[2]);
        strcpy(input_file, argv[3]);
        LifeBoard* board = malloc(sizeof(LifeBoard));
        FILE* in = fopen(input_file, "r");
        read_life_board(in, board);

        // struct timeval start_time, end_time;
        // gettimeofday(&start_time, NULL);
        
        simulate_life_parallel(threads, board, steps);

        // gettimeofday(&end_time, NULL);
        // long seconds = end_time.tv_sec - start_time.tv_sec;
        // long microseconds = end_time.tv_usec - start_time.tv_usec;
        // double elapsed_time = seconds * 1000 + microseconds / 1000.0;
        // printf("Use time: %.5fs\n", elapsed_time/1000);
        if(NEED_PRINT){
            print_life_board(board);
        }
        destroy_life_board(board);
        return 0;
    }else{
        printf("Usage:\n"
               "  %s STEPS FILENAME\n"
               "    Run serial computation and print out result\n", argv[0]);
        return 1;
    }
}


// int main(int argc, char** argv) {
//     int steps = 0;
//     int threads = 1;
//     char input_file[256];
//     if (argc == 4) {
//         threads = atoi(argv[1]);
//         steps = atoi(argv[2]);
//         strcpy(input_file, argv[3]);
//     }
//     if (argc != 4) {
//         printf("Usage:\n"
//                "  %s STEPS FILENAME\n"
//                "    Run serial computation and print out result\n", argv[0]);
//         return 1;
//     }
//     LifeBoard* board = malloc(sizeof(LifeBoard));
//     FILE* in = fopen(input_file, "r");
//     read_life_board(in, board);
//     simulate_life_parallel(threads, board, steps);
//     print_life_board(board);
//     destroy_life_board(board);
//     return 0;
// }


// int main(int argc, char** argv) {
//     int steps = 0;
//     char input_file[256];
//     if (argc == 3) {
//         steps = atoi(argv[1]);
//         strcpy(input_file, argv[2]);
//     }
//     if (argc != 3) {
//         printf("Usage:\n"
//                "  %s STEPS FILENAME\n"
//                "    Run serial computation and print out result\n", argv[0]);
//         return 1;
//     }
//     LifeBoard* board = malloc(sizeof(LifeBoard));
//     FILE* in = fopen(input_file, "r");
//     read_life_board(in, board);
//     simulate_life_serial(board, steps);
//     print_life_board(board);
//     destroy_life_board(board);
//     return 0;
// }
