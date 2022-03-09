#include "SBFSHelper.h"
#include <stdio.h>
#include "SBFS.h"
#include <string.h>
#include <assert.h>
#include <time.h>

uint64_t get_nanos(void) {
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return (uint64_t)ts.tv_sec * 1000000000L + ts.tv_nsec;
}

uint64_t read_symlink(uint64_t symlink_inum) {
    uint64_t inum = symlink_inum;
    while (1) {
        char buf[400];
        inode node;
        read_inode(inum, &node);
        if ((node.permission_bits & FILEMASK) >> 12 != SYMBOLIC) {
            return inum;
        }
        read_block(node.direct_blocks[0], 0, node.size, buf);
        inum = SBFS_namei(buf);
        if (inum == 0)
            return 0;
        break;
    }
}

uint64_t find_parent_dir_inum(char *path)
{
    char parent_path[MAX_PATH];
    int len = strlen(path);
    if(len>MAX_PATH)
        return 0;
    while (*(path + len - 1) != '/'){
        len--;
    }
    memcpy(parent_path,path,len);
    if(len==1)
        parent_path[len] = 0;
    else
        parent_path[len-1] = 0;
    char* filename = path + len;
    printf("path %s filename %s\n",parent_path,filename);
    uint64_t parent_path_inum = SBFS_namei(parent_path);
    return parent_path_inum;
}

void setFiletype(inode* node,int type){
    node->permission_bits = node->permission_bits & 0x8fff;
    type = type << 12;
    node->permission_bits = node->permission_bits | type;
    node->last_access_time = node->last_modify_time = get_nanos();
}

char *SBFS_clean_path(char *path)
{
    int state = 0;
    char tmp[4096];
    int i = 0;
    while (*path != 0)
    {
        if (state == 0)
        {
            if (*path == '/')
            {
                state = 1;
            }
            tmp[i++] = *path;
        }
        else
        {
            if (*path != '/')
            {
                tmp[i++] = *path;
                state = 0;
            }
        }
        path += 1;
    }
    tmp[i] = 0;
}
void SBFS_readdir_raw(uint64_t inum, int entry_index, dir *entry)
{
    SBFS_read(inum, entry_index * sizeof(dir), sizeof(dir), entry);
}

uint64_t add_entry_to_dir(uint64_t dir_inum, char *entryname, uint64_t file_inum)
{
    inode node;
    dir entry;
    read_inode(dir_inum, &node);
    int entries = node.size / sizeof(dir);
    assert(node.size % sizeof(dir) == 0);
    int i = 0;
    for (i = 0; i < entries; i++)
    {
        SBFS_readdir_raw(dir_inum, i, &entry);
        if (entry.inum == 0)
        {
            strcpy(entry.filename, entryname);
            entry.inum = file_inum;
            uint64_t tmp = node.size;
            SBFS_write(dir_inum, i * sizeof(entry), sizeof(entry), &entry);
            read_inode(dir_inum, &node);
            node.size = tmp;
            write_inode(dir_inum,&node);
            return i;
        }
    }
    strcpy(entry.filename, entryname);
    entry.inum = file_inum;
    uint64_t tmp = node.size;
    SBFS_write(dir_inum, i * sizeof(entry), sizeof(entry), &entry);
    read_inode(dir_inum, &node);
    assert((tmp+sizeof(entry))==node.size);
    return i;
}

int delete_entry_from_dir(uint64_t dir_inum, int index)
{
    inode node;
    dir entry;
    read_inode(dir_inum, &node);
    assert(node.size % sizeof(dir) == 0);
    SBFS_readdir_raw(dir_inum, index, &entry);
    entry.inum = 0;
    uint64_t tmp = node.size;
    SBFS_write(dir_inum, index * sizeof(entry), sizeof(entry), &entry);
    read_inode(dir_inum, &node);
    node.size = tmp;
    write_inode(dir_inum,&node);
    return 1;
}
/**
 * index -1 means failed
 * **/
int find_file_entry(uint64_t inum, char *entryname,dir* ety)
{
    inode node;
    read_inode(inum, &node);
    if(strlen(entryname)>MAX_FILENAME)
        return -1;
    assert( (node.permission_bits & FILEMASK) >> 12 == DIR);
    int entry_count = node.size / sizeof(dir);
    dir entry;
    assert((node.size % sizeof(dir)) == 0);
    for (int i = 0; i < entry_count; i++)
    {
        SBFS_readdir_raw(inum, i, &entry);
        if (strcmp(entryname, entry.filename) == 0 && entry.inum != 0) {
            if(ety != NULL){
                ety->inum = entry.inum;
                strcpy(ety->filename,entry.filename);
            }
            return i;
        }
    }
    return -1;
}

int get_entryname(char* path,char* entryname){
    int len = strlen(path);
    if(path[len-1] == '/')
        return 0;
    else{
        while(path[len-1] != '/'){
            len--;
        }
        if(strlen(path+len)>MAX_FILENAME)
            return 0;
        strcpy(entryname,path+len);
        return 1;
    }
}

int write_entry(uint64_t dir_inum, int index, dir* entry){
    inode node;
    read_inode(dir_inum, &node);
    assert((node.permission_bits & FILEMASK) >> 12 == DIR);
    uint64_t tmp = node.size;
    SBFS_write(dir_inum,index*sizeof(dir),sizeof(dir),entry);
    read_inode(dir_inum, &node);
    node.size = tmp;
    write_inode(dir_inum,&node);
    return 1;
}

uint64_t block_id_helper(inode *node, int index, int mode)
{
    //printf("block id helper called\n");
    char tmp[BLOCKSIZE];
    char zero[BLOCKSIZE];
    memset(zero, 0, BLOCKSIZE);
    uint64_t *address = (uint64_t *)tmp;
    if (index < DIRECT_BLOCK)
    {
        //printf("allocated node->direct_blocks[%d] %ld\n",index,node->direct_blocks[index]);
        if (node->direct_blocks[index] == 0 && mode == H_CREATE)
        {
            node->direct_blocks[index] = allocate_data_block();
            //printf("allocated node->direct_blocks[%d] %ld\n",index,node->direct_blocks[index]);
        }
        //printf("return node->direct_blocks[%d] %ld\n",index,node->direct_blocks[index]);
        return node->direct_blocks[index];
    }
    else if (index < (DIRECT_BLOCK + SING_INDIR * 512))
    {
        printf("\n\n\nsecond phase\n");
        if (node->sing_indirect_blocks[0] == 0 && mode == H_CREATE)
        {
            if (mode == H_CREATE)
            {
                node->sing_indirect_blocks[0] = allocate_data_block();
                write_block(node->sing_indirect_blocks[0], 0, BLOCKSIZE, zero);
            }
            else
                return 0;
        }
        // I/O can be cut down, but I will leave it for now
        read_block(node->sing_indirect_blocks[0], 0, BLOCKSIZE, tmp);
        if (address[index - DIRECT_BLOCK] == 0 && mode == H_CREATE)
        {
            address[index - DIRECT_BLOCK] = allocate_data_block();
            write_block(node->sing_indirect_blocks[0], 0, BLOCKSIZE, tmp);
        }
        printf("address[%ld] = %ld\n\n\n",index-DIRECT_BLOCK,address[index - DIRECT_BLOCK]);
        return address[index - DIRECT_BLOCK];
    }
    else if (index < (DIRECT_BLOCK + SING_INDIR * 512 + DOUB_INDIR * 512 * 512))
    {
        if (node->doub_indirect_blocks[0] == 0)
        {
            if (mode == H_CREATE)
            {
                node->doub_indirect_blocks[0] = allocate_data_block();
                write_block(node->doub_indirect_blocks[0], 0, BLOCKSIZE, zero);
            }
            else
                return 0;
        }
        read_block(node->doub_indirect_blocks[0], 0, BLOCKSIZE, tmp);
        int next_level_index = (index - DIRECT_BLOCK - SING_INDIR * 512) / 512;
        if (address[next_level_index] == 0)
        {
            if (mode == H_CREATE)
            {
                address[next_level_index] = allocate_data_block();
                write_block(node->doub_indirect_blocks[0], 0, BLOCKSIZE, tmp);
                write_block(address[next_level_index], 0, BLOCKSIZE, zero);
            }
            else
                return 0;
        }
        int parent_id = address[next_level_index];
        read_block(address[next_level_index], 0, BLOCKSIZE, tmp);
        index = (index - DIRECT_BLOCK - SING_INDIR * 512) % 512;
        if (address[index] == 0 && mode == H_CREATE)
        {
            address[index] = allocate_data_block();
            write_block(parent_id, 0, BLOCKSIZE, tmp);
        }
        return address[index];
    }
    else
    {
        if (node->trip_indirect_blocks[0] == 0)
        {
            if (mode == H_CREATE)
            {
                node->trip_indirect_blocks[0] = allocate_data_block();
                write_block(node->trip_indirect_blocks[0], 0, BLOCKSIZE, zero);
            }
            else
                return 0;
        }
        read_block(node->trip_indirect_blocks[0], 0, BLOCKSIZE, tmp);
        int next_level_index = (index - DIRECT_BLOCK - SING_INDIR * 512) / (512 * 512);
        if (address[next_level_index] == 0)
        {
            if (mode == H_CREATE)
            {
                address[next_level_index] = allocate_data_block();
                write_block(address[next_level_index], 0, BLOCKSIZE, zero);
                write_block(node->trip_indirect_blocks[0], 0, BLOCKSIZE, tmp);
            }
            else
                return 0;
        }
        int parent_id = address[next_level_index];
        read_block(address[next_level_index], 0, BLOCKSIZE, tmp);
        next_level_index = (index - DIRECT_BLOCK - SING_INDIR * 512) / 512;
        if (address[next_level_index] == 0)
        {
            if (mode == H_CREATE)
            {
                address[next_level_index] = allocate_data_block();
                write_block(address[next_level_index], 0, BLOCKSIZE, zero);
                write_block(parent_id, 0, BLOCKSIZE, tmp);
            }
            else
                return 0;
        }
        parent_id = address[next_level_index];
        read_block(address[next_level_index], 0, BLOCKSIZE, tmp);
        index = (index - DIRECT_BLOCK - SING_INDIR * 512) % 512;
        if (address[index] == 0 && mode == H_CREATE)
        {
            address[index] = allocate_data_block();
            write_block(parent_id, 0, BLOCKSIZE, tmp);
        }
        return address[index];
    }
}

int checkExcl(const inode* node, unsigned int userId,unsigned int groupId){
    if (userId == node->owner) {
        if (((node->permission_bits & OWNERMASK) >> 6 & 0x1)) {
            return 1;
        }
    }
    if(groupId == node->group){
        if( (node->permission_bits & GROUPMASK) >> 3 & 0x1)
            return 1;
    }
    if(node->permission_bits & 0x1)
        return 1;
    return 0;
}

int checkWrite(const inode* node, unsigned int userId,unsigned int groupId){

    if (userId == node->owner) {
        if (((node->permission_bits & OWNERMASK) >> 7 & 0x1)) {
            return 1;
        }
    }
    if(groupId == node->group){
        if( (node->permission_bits & GROUPMASK) >> 4 & 0x1)
            return 1;
    }
    if((node->permission_bits & WORLDMASK) >> 1 & 0x1)
        return 1;
    return 0;
}

int checkRead(const inode* node, unsigned int userId,unsigned int groupId){
    if (userId == node->owner) {
        if (((node->permission_bits & OWNERMASK) >> 8 & 0x1)) {
            return 1;
        }
    }
    if(groupId == node->group){
        if( (node->permission_bits & GROUPMASK) >> 5 & 0x1)
            return 1;
    }
    if((node->permission_bits & WORLDMASK) >> 2 & 0x1)
        return 1;
    return 0;
}

int getMountPoint(char* buf,size_t size) {
    char line[128];
    size_t line_size = 128;
    FILE *fp = fopen("/proc/mounts", "r");
    if(fp==NULL)
        return 0;
    while (fgets(line, 128, fp)) {
        printf("%s\n",line);
        if (line[0] == 'm' && line[1] == 'a' && line[2] == 'i' && line[3] == 'n' && line[4] == ' ') {
            int i = 5;
            while (line[i] != ' ' && (i - 5) < size && line[i] != 0) {
                buf[i - 5] = line[i];
                i++;
            }
            buf[i - 5] = 0;
            fclose(fp);
            return 1;
        }
    }
    fclose(fp);
    return 0;
}