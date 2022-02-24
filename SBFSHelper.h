//
// Created by xie on 2022/2/20.
//
#ifndef CS270_SBFSHELPER_H
#define CS270_SBFSHELPER_H

#include "SBFSStruct.h"
#include <stdint.h>

uint64_t get_nanos(void);
uint64_t read_symlink(uint64_t symlink_inum);
void setFiletype(inode* node,int type);
char *SBFS_clean_path(char *path);
void SBFS_readdir_raw(uint64_t inum, int entry_index, dir *entry);
uint64_t add_entry_to_dir(uint64_t dir_inum, char *entryname, uint64_t file_inum);
int delete_entry_from_dir(uint64_t dir_inum, int index);
int find_file_entry(uint64_t inum, char *entryname, dir* ety);
int write_entry(uint64_t dir_inum, int index, dir* entry);
int get_entryname(char* path,char* entryname);
uint64_t find_parent_dir_inum(char *path);
uint64_t block_id_helper(inode *node, int index, int mode);
void create_root_dir();

#endif //CS270_SBFSHELPER_H
