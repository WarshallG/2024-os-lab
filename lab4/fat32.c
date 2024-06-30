#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <sys/mman.h> 
#include "fat32.h"

struct Fat32BPB *hdr; // 指向 BPB 的数据
int mounted = -1; // 是否已挂载成功

#define MAX_OPEN_FILES 128
#define MAX_PATH_LENGTH 128

// 定义打开文件结构体
typedef struct {
    char path[MAX_PATH_LENGTH];
    // 其他文件相关信息可以在这里添加
    int fd;
    int first_cluster;
    int is_open;
    int size;
    int offset;
} OpenFile;

// 定义全局打开文件表和文件计数器
OpenFile open_files[MAX_OPEN_FILES];
int num_open_files = 0;
int max_fd = -1;

int Is_match(char *token, uint8_t *filename);

uint32_t get_starting_cluster(struct DirEntry *entry) {
    uint32_t cluster = (entry->DIR_FstClusHI << 16) | entry->DIR_FstClusLO;
    return cluster;
}

uint32_t get_FAT_content(uint32_t cluster){
    return *(uint32_t *)((void *)hdr + hdr->BPB_BytsPerSec * hdr->BPB_RsvdSecCnt + 4 * cluster) & 0x0FFFFFFF;
}

struct DirEntry *get_the_direntry(uint32_t cluster){
    return (struct DirEntry *)((void *)hdr + hdr->BPB_RsvdSecCnt * hdr->BPB_BytsPerSec + 
        hdr->BPB_NumFATs * hdr->BPB_FATSz32 * hdr->BPB_BytsPerSec + (cluster - 2) * hdr->BPB_SecPerClus 
        * hdr->BPB_BytsPerSec + 0 * sizeof(struct DirEntry));
}

int Is_last_cluster(uint32_t cluster){
    if(cluster == 0x0FFFFFF7){
        // printf("Bad Cluster\n");
        return -1;
    }else if(cluster >= 0x0FFFFFF8 && cluster <= 0x0FFFFFFF){
        return 1;
    }
    return 0;
}

struct FilesInfo* get_filesinfo(uint32_t cluster){
    //给定目录文件的簇号，返回所有文件信息
    if(Is_last_cluster(cluster) == -1){
        return NULL;  //坏簇
    }
    struct FilesInfo *files_info = (struct FilesInfo *)malloc(sizeof(struct FilesInfo));
    while(1){
        struct DirEntry *direntry = get_the_direntry(cluster);

        struct FileInfo file_info;
        int index = 0;
        while(direntry->DIR_Name[0] != 0x00 && index < hdr->BPB_BytsPerSec * hdr->BPB_SecPerClus / sizeof(struct DirEntry)){
            if(direntry->DIR_Name[0] == 0xE5 || direntry->DIR_Attr == LONG_NAME){
                direntry = (struct DirEntry * )((void *)direntry + sizeof(struct DirEntry));
                index ++;
                continue;
            }
            if(direntry->DIR_Name[0] == 0x20){
                // printf("Error: filename cannot start with a blank!\n");
                free(files_info->files);
                free(files_info);
                return NULL;
            }

            memcpy(file_info.DIR_Name, direntry->DIR_Name, 11);
            file_info.DIR_FileSize = direntry->DIR_FileSize;
            struct FileInfo *new_files = (struct FileInfo *)realloc(files_info->files, (files_info->size + 1) * sizeof(struct FileInfo));
            if (new_files == NULL) {
                // 内存分配失败，释放之前分配的空间并返回 NULL
                free(files_info->files);
                free(files_info);
                return NULL;
            }

            files_info->files = new_files;
            memcpy(&files_info->files[files_info->size], &file_info, sizeof(struct FileInfo));
            files_info->size ++;
            //下一个目录项
            direntry = (struct DirEntry * )((void *)direntry + sizeof(struct DirEntry));
            index ++;
        }
        cluster = get_FAT_content(cluster); //得到当前簇在FAT表中对应的表项
        if(Is_last_cluster(cluster) == 1){
            break;
        }
    }
    return files_info;
}

uint32_t get_cluster_from_token(uint32_t cur_cluster, char *token){
    //给定一个token，例如/DIR_2/PROPLE，返回簇号
    while (1){
        struct DirEntry *cur_direntry = get_the_direntry(cur_cluster); //得到当前簇的数据区指针
        int index = 0;
        while(cur_direntry->DIR_Name[0] != 0x00 && index < hdr->BPB_BytsPerSec * hdr->BPB_SecPerClus / sizeof(struct DirEntry)){
            if(cur_direntry->DIR_Name[0] == 0xE5 || cur_direntry->DIR_Attr == LONG_NAME){
                cur_direntry = (struct DirEntry *)((void *)cur_direntry + sizeof(struct DirEntry));
                index ++;
                continue;
            }
            if(cur_direntry->DIR_Name[0] == 0x20){
                // printf("Error: filename cannot start with a blank!\n");
                return 0;
            }
            if(Is_match(token, cur_direntry->DIR_Name)){
                return get_starting_cluster(cur_direntry); //找到了与token一致的目录项
            }
            cur_direntry = (struct DirEntry *)((void *)cur_direntry + sizeof(struct DirEntry));
            index ++;
        }
        cur_cluster = get_FAT_content(cur_cluster); //当前的簇读完了，读下一个簇的簇号
        if(Is_last_cluster(cur_cluster) == 1){
            return 0; //如果是最后一个簇了，但是还没找到，就返回0作为错误码
        }
    }
    

}

uint32_t get_the_cluster(const char *path){ 
    //通过给定的路径path，返回对应的第一个簇号
    char path_copy[strlen(path) + 1];
    strcpy(path_copy, path);
    char *token = strtok(path_copy, "/");
    uint32_t cur_cluster = hdr->BPB_RootClus; //根目录起始簇
    while(token != NULL){
        cur_cluster = get_cluster_from_token(cur_cluster, token);
        if(cur_cluster == 0){ //没找到，返回0
            // printf("The directory doesn't exist!\n");
            return 0;
        }
        token = strtok(NULL, "/");
    }
    return cur_cluster;
}

int format_fat32_filename(char *filename) {
    // 初始化格式化后的文件名数组
    char formatted[12];
    memset(formatted, ' ', 11);
    formatted[11] = '\0';

    // 找到文件名中的点，分隔文件名和扩展名
    char *dot = strrchr(filename, '.');
    if(dot == NULL){
        return -1;
    }
    int name_len = dot ? (dot - filename) : strlen(filename);
    int ext_len = dot ? strlen(dot + 1) : 0;

    // 填充文件名部分，最多 8 个字符
    for (int i = 0; i < name_len && i < 8; ++i) {
        formatted[i] = toupper((unsigned char)filename[i]);
    }

    // 填充扩展名部分，最多 3 个字符
    if (dot && ext_len > 0) {
        for (int i = 0; i < ext_len && i < 3; ++i) {
            formatted[8 + i] = toupper((unsigned char)dot[1 + i]);
        }
    }

    // 将格式化后的文件名拷贝回原位置
    strncpy(filename, formatted, 12);
    return 0;
}

void split_path(const char *path, char *dir_path, char *file_name) {
    // 拷贝原路径到临时缓冲区以便操作
    char temp_path[128];
    strcpy(temp_path, path);

    // 找到最后一个斜杠的位置
    char *last_slash = strrchr(temp_path, '/');

    // 如果找到了斜杠，将其分割
    if (last_slash != NULL) {
        // 获取文件名
        strcpy(file_name, last_slash + 1);
        // 截断目录路径
        *last_slash = '\0';
        strcpy(dir_path, temp_path);
    } 
}

// 挂载磁盘镜像
int fat_mount(const char *path) {
    // 只读模式打开磁盘镜像
    int fd = open(path, O_RDWR);
    if (fd < 0){
        // 打开失败
        return -1;
    }
    // 获取磁盘镜像的大小
    off_t size = lseek(fd, 0, SEEK_END);
    if (size == -1){
        // 获取失败
        return -1;
    }
    // 将磁盘镜像映射到内存
    hdr = (struct Fat32BPB *)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (hdr == (void *)-1){
        // 映射失败
        return -1;
    }
    // 关闭文件
    close(fd);

    assert(hdr->Signature_word == 0xaa55); // MBR 的最后两个字节应该是 0x55 和 0xaa
    assert(hdr->BPB_TotSec32 * hdr->BPB_BytsPerSec == size); // 不相等表明文件的 FAT32 文件系统参数可能不正确

    // 打印 BPB 的部分信息
    printf("Some of the information about BPB is as follows\n");
    printf("OEM-ID \"%s\", \n", hdr->BS_oemName);
    printf("sectors/cluster %d, \n", hdr->BPB_SecPerClus);
    printf("bytes/sector %d, \n", hdr->BPB_BytsPerSec);
    printf("sectors %d, \n", hdr->BPB_TotSec32);
    printf("sectors/FAT %d, \n", hdr->BPB_FATSz32);
    printf("FATs %d, \n", hdr->BPB_NumFATs);
    printf("reserved sectors %d, \n", hdr->BPB_RsvdSecCnt);
    // 一些数据
    // 每个簇（数据块）有8个sectors
    // 每个sectors有512个字节
    // 也就是说每个簇（数据块）为4KB.
    // 每个FAT表有128个sectors 
    // 有2个FAT表
    // reserved sectors 有32个
    // _ ... _ | _ ... _ | _ ... _ | _ ... _  
    //  保留扇区    FAT1      FAT2      数据区

    // 挂载成功
    mounted = 0;
    return 0;
}


// 打开文件
int fat_open(const char *path) {
    // 检查是否成功挂载 FAT32 文件系统
    if (mounted != 0) {
        return -1; // 挂载失败，返回错误
    }

    char path_copy[strlen(path) + 1];
    strcpy(path_copy, path);
    char *last_slash = strrchr(path_copy, '/'); // 找到最后一个斜杠的位置
    if (last_slash == NULL ) {
        return -1; // 路径无效，返回错误
    }

    // 提取文件名部分
    char *file_name = last_slash + 1;

    // 格式化文件名为 FAT32 短文件名格式
    int isnot_dir = format_fat32_filename(file_name);
    char target_file[12];
    strcpy(target_file, file_name); //保存下FAT格式的文件名，为后面找到文件大小准备

    if(isnot_dir == -1){  //打开的是目录文件，返回-1
        return -1;
    }

    uint32_t cluster = get_the_cluster(path_copy); //至此，我们得到了该普通文件的数据区的第一个簇号
    if(cluster == 0){ //打开失败
        return -1;
    }

    int slot = -1;  //找空闲槽位
    if(num_open_files == 0){
        slot = 0;
    }else{
        for(int i = 0; i < MAX_OPEN_FILES; i++){
            if(open_files[i].is_open == 0){
                slot = i;
                break;
            }
        }
        if(slot == -1){
            //找不到空闲的位置
            return -1;
        }
    }

    
    strcpy(open_files[slot].path, path);
    // printf("open_files[slot].path: %s\n", open_files[slot].path);
    open_files[slot].is_open = 1;
    open_files[slot].first_cluster = cluster;
    open_files[slot].fd = slot; //直接将slot作为fd
    open_files[slot].offset = 0; //偏移值初始化为0
    num_open_files++;
    max_fd = (max_fd > slot) ? max_fd : slot;

    // 解析出该文件的上级目录路径

    *last_slash = '\0';
    if(path_copy[0] == '\0'){
        path_copy[0] = '/';
        path_copy[1] = '\0';
    }

    // 调用fat_readdir得到该文件的上级目录信息，从而得到该文件大小信息
    struct FilesInfo *files_info = fat_readdir(path_copy);
    for(int i = 0; i < files_info->size; i++){
        if(memcmp(target_file, files_info->files[i].DIR_Name, 11) == 0){
            open_files[slot].size = files_info->files[i].DIR_FileSize;
            break;
        }
    }


    // for(int i = 0; i < max_fd + 1; i++){
    //     printf("path: %s, fd: %d, cluster: %d, is_open: %d, size: %d\n", open_files[i].path, 
    //     open_files[i].fd, open_files[i].first_cluster, open_files[i].is_open, open_files[i].size);
    // }
    // printf("\n");
    // printf("%d\n", max_fd);
    return slot;
}

// 关闭文件
int fat_close(int fd) {
    if (mounted != 0) {
        return -1; // 挂载失败，返回错误
    }
    if(fd < 0){
        return -1;
    }

    if(fd > max_fd){
        return -1;
    }

    if(open_files[fd].is_open == 0){
        // printf("The file isnot open, fd = %d\n", fd);
        return -1;
    }else{
        open_files[fd].is_open = 0;
    }

    return 0;
}

// 读取普通文件内容
int fat_pread(int fd, void *buffer, int count, int offset) {
    if (mounted != 0) {
        return -1; // 挂载失败，返回错误
    }
    if(fd > max_fd){
        return -1; //没有这个文件
    }

    if(open_files[fd].is_open == 0){
        return -1; //文件没有打开
    }

    if(count == 0 || offset >= open_files[fd].size){
        return 0;
    }

    int cluster_size = hdr->BPB_BytsPerSec * hdr->BPB_SecPerClus;
    int final_offset = (offset + count > open_files[fd].size ) ? (open_files[fd].size - 1) : (offset + count - 1);
    int offset_cluster = ((offset + 1) % cluster_size == 0) ? ((offset + 1) / cluster_size) : ((offset + 1) / cluster_size + 1);
    int final_offset_cluster = ((final_offset + 1) % cluster_size == 0) ? ((final_offset + 1) / cluster_size) : ((final_offset + 1) / cluster_size + 1);
    // printf("offset: %d, final_offset: %d\n", offset, final_offset);
    // printf("offset_cluster: %d, final_offset_cluster: %d\n", offset_cluster, final_offset_cluster);
    // printf("\n");

    // 找到offset处的簇号
    uint32_t start_cluster = open_files[fd].first_cluster;
    uint32_t final_cluster = open_files[fd].first_cluster;
    for(int i = 1; i < offset_cluster; i++){
        start_cluster = get_FAT_content(start_cluster);
    } // offset的簇号
    for(int i = 1; i < final_offset_cluster; i++){
        final_cluster = get_FAT_content(final_cluster);
    } // final_offset处的簇号

    int start_offset = offset % cluster_size;
    int end_offset = final_offset % cluster_size;

    char *p = (char *)((void *)get_the_direntry(start_cluster));
    char *q = (char *)((void *)get_the_direntry(final_cluster));
    int first = 1;
    void *cur_position = buffer;
    while(p != q){
        if(first == 1){
            // 说明是第一个簇
            memcpy(cur_position, p + start_offset, cluster_size - start_offset);
            cur_position += cluster_size - start_offset;
            first = 0;
            start_cluster = get_FAT_content(start_cluster);
            p = (char *)((void *)get_the_direntry(start_cluster));
            start_offset = 0;
            continue;
        }
        // 不是第一个簇
        memcpy(cur_position, p, cluster_size);
        cur_position += cluster_size;
        start_cluster = get_FAT_content(start_cluster);
        p = (char *)((void *)get_the_direntry(start_cluster));
    }
    memcpy(cur_position, p + start_offset, end_offset - start_offset + 1);

    open_files[fd].offset = final_offset + 1;
    return final_offset - offset + 1;
}

int Is_match(char *token, uint8_t *filename){
    if (token == NULL || filename == NULL) {
        return 0; // 如果传入空指针，直接返回不匹配
    }
    int token_len = strlen(token);
    if(token_len > 11){
        return 0;
    }

    for(int i = 0; i < token_len ; i++){
        if(token[i] != filename[i]){
            return 0;
        }
    }

    for(int i = token_len; i < 11; i++){
        if(filename[i] != ' '){
            return 0;
        }
    }
    return 1;
}

// 读取目录文件内容 (目录项)
struct FilesInfo* fat_readdir(const char *path) {
    if (mounted != 0) {
        return NULL; // 挂载失败，返回错误
    }
    if (path == NULL || path[0] != '/') {
        return NULL;
    }

    if(strcmp(path,"/") == 0){
        return get_filesinfo(hdr->BPB_RootClus); // Root Directory.
    }else{
        uint32_t first_cluster= get_the_cluster(path);
        if(first_cluster == 0){
            return NULL; //没找到
        }
        return get_filesinfo(first_cluster);
    }
    return NULL;
}
