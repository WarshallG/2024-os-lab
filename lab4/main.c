#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "fat32.h"

void print(uint8_t *filename, int n ){
    for(int i = 0; i < n; i++){
        printf("%c", filename[i]);
    }
    printf("\n");
}

int main(){
    // struct FilesInfo *files_info = fat_readdir("/");
    int mounted = fat_mount("fat32.img");
    assert(mounted == 0);
    // struct FilesInfo *files_info = fat_readdir("/DIR_1");

    // int fd = fat_open("/exam_1.txt");
    // printf("fd: %d\n", fd);
    // // fat_open("/DIR_1/555.txt");
    // assert(fd != -1);


    // fat_close(0);
    // fat_open("/exam_1.txt");

    // int fd = fat_open("/DIR_2/DIR_1/DIR_1/DIR_1/DIR_1/DIR_1/DIR_1/test_2.txt");
    // printf("fd: %d\n", fd);


    // char *buffer = (char *)malloc(128 * sizeof(char));
    // int Read_Size = fat_pread(1, buffer, 63, 10);
    // printf("%d\n", Read_Size);
    // printf("%d\n", fat_close(1));
    // printf("%d\n", fat_close(0));
    // free(buffer);
    // assert(Read_Size == 3);
    // assert(strcmp(buffer, "The") == 0);


    // struct FilesInfo *files_info = fat_readdir("/DIR_2/PEOPLE");
    // if(files_info == NULL){
    //     printf("Fail to get the files info\n");
    //     return 0;
    // }
    // for(int i=0 ; i<files_info->size; i++){
    //     print(files_info->files[i].DIR_Name, 11);
    //     printf("size is: %d\n", files_info->files[i].DIR_FileSize);
    // }

    // if(files_info != NULL){
    //     if(files_info->files != NULL){
    //         free(files_info->files);
    //     }
    //     free(files_info);
    // }
    return 0;
}

