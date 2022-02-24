#include "SBFS.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include "SBFSHelper.h"


#define MIN(a, b) ((a > b) ? b : a)
#define MAX(a, b) ((a > b) ? a : b)

int SBFS_getattr(char* path, struct stat* file_state){
    uint64_t inum = SBFS_namei(path);
    if(inum==0)
        return 0;
    inode node;
    read_inode(inum,&node);
    file_state->st_gid = node.group;
    file_state->st_uid = node.owner;
    file_state->st_size = node.size;
    file_state->st_nlink = node.link;
    file_state->st_atim.tv_nsec = node.last_access_time % 1000000000L;
    file_state->st_atim.tv_sec = node.last_access_time / 1000000000L;
    file_state->st_mtim.tv_nsec = node.last_modify_time % 1000000000L;
    file_state->st_mtim.tv_sec = node.last_modify_time / 1000000000L;
    file_state->st_ctim.tv_nsec = 0;
    file_state->st_ctim.tv_sec = 0;
    return 1;
}
/**
 * failed -1
 * success the size of the buf
 * **/
unsigned long SBFS_readlink(char* path,char* buf) {
    uint64_t inum = SBFS_namei(path);
    if (inum == 0)
        return -1;
    inode node;
    read_inode(inum, &node);
    if ((node.permission_bits & FILEMASK) >> 12 != SYMBOLIC) {
        return -1;
    }
    read_block(node.direct_blocks[0], 0, node.size, buf);
    return node.size;
}

uint64_t SBFS_namei(char *path) {
    char filename[MAX_FILENAME];
    char *pointer = path;
    int i = 0;
    uint64_t inum;
    dir tmp;
    if (strcmp(path, "/") == 0) {
        return ROOT;
    }
    pointer += 1;
    inum = ROOT;

    while (*pointer != 0) {
        while (*pointer != '/' && *pointer != 0) {
            filename[i++] = *pointer;
            pointer += 1;
        }
        filename[i] = 0;
        if(find_file_entry(inum, filename, &tmp)==-1)
            return 0;
        printf("tmp.inum %ld\n",tmp.inum);
        inum = tmp.inum;
        if (inum == 0) {
            printf("namei, failed to find path: %s %s\n", path, filename);
            return 0; //cannot find inode
        }
        //changed!!
        if (*pointer == '/') {
            inode node;
            read_inode(inum, &node);
            int filetype = (node.permission_bits & FILEMASK) >> 12;
            printf("file type %d\n",filetype);
            if (filetype == SYMBOLIC) {
                uint64_t target_inum = read_symlink(inum);
                if (target_inum == 0) {
                    printf("link file has been moved or deleted\n");
                    return 0;
                }
                read_inode(target_inum, &node);
                if ((node.permission_bits & FILEMASK) >> 12 != DIR) {
                    printf("%s it is not a dir\n", filename);
                    return 0;
                }
                inum = target_inum;
            } else if (filetype != DIR) {
                printf("%s it is not a dir\n", filename);
                return 0;
            }
        }
        while (*pointer == '/') {
            i = 0;
            pointer++;
        }
        if (*pointer == 0)
            return inum;
    }
    return 1; // the path end with /
}

uint64_t SBFS_read(uint64_t inum, uint64_t offset, int64_t size, void *buf)
{
	char *buffer = (char *)buf;
	inode node;
	read_inode(inum, &node);
    printf("inum %ld size %ld\n",inum,node.size);
    if(node.size<offset){
        strcpy(buf,"");
        return 0;
    }
	uint64_t read_size, pre_size;
    read_size = pre_size = MIN(size,node.size-offset);

	int start = block_id_helper(&node, offset / BLOCKSIZE, H_READ);
	uint64_t block_offset = offset % BLOCKSIZE;
	assert(block_offset < BLOCKSIZE);

	int read_bytes = read_block(start, block_offset, read_size, buf);
	read_size -= read_bytes;
	buffer += read_bytes;

	while (read_size > 0)
	{
		start += 1;
		int block_id = block_id_helper(&node, start, H_CREATE);
		assert(block_id != 0);
		read_bytes = read_block(block_id, block_offset, size, buf);
		buffer += read_bytes;
		read_size -= read_bytes;
	}
	return pre_size;
}
/* mode H_READ, don't create new block,mode H_CREATE create new block
 * the corresponding inode where and what size
 * */

int SBFS_write(uint64_t inum, uint64_t offset, int64_t size, void *buf)
{
	inode node;
	read_inode(inum, &node);

	char *buffer = (char *)buf;
	uint64_t upperbound = size + offset;
	int start = block_id_helper(&node, offset / BLOCKSIZE, H_CREATE);
	uint64_t block_offset = offset % BLOCKSIZE;
	assert(block_offset < BLOCKSIZE);

	int write_bytes = write_block(start, block_offset, size, buf);
	size -= write_bytes;
	buffer += write_bytes;

	while (size > 0)
	{
		start += 1;
		int block_id = block_id_helper(&node, start, H_CREATE);
		write_bytes = write_block(block_id, block_offset, size, buf);
		buffer += write_bytes;
		size -= write_bytes;
	}

	node.size = MAX(node.size, upperbound);
	write_inode(inum, &node);
	read_inode(inum, &node);
	return size;
}

/* direcotry is dir, the item is empty when inode = 0 */
/*
return 0: can not mkdir
*/
uint64_t SBFS_mkdir(char *path)
{
	char parent_path[5 * MAX_FILENAME];
	if (SBFS_namei(path) != 0)
	{
		return 0;
	}
	int len = strlen(path);
	int i = 0;
	while (*(path + len - 1) != '/')
	{
		len--;
	}
	memcpy(parent_path, path, len);
	if (len == 1)
		parent_path[len] = 0;
	else
		parent_path[len - 1] = 0;
	char *filename = path + len;
	printf("path %s filename %s\n", parent_path, filename);
	uint64_t parent_path_inum = SBFS_namei(parent_path);
	uint64_t child_inum;
	if (parent_path_inum == 0)
	{
		return 0;
	}
	else
	{
		inode parent_node;
		read_inode(parent_path_inum, &parent_node);
		if ( (parent_node.permission_bits & FILEMASK) >> 12 != DIR)
		{
			printf("path:%s not a dir\n", parent_path);
			return 0;
		}
		else
		{
			inode node;
			child_inum = allocate_inode();
			read_inode(child_inum, &node);
            setFiletype(&node,DIR);
			node.size = 0;
			write_inode(child_inum, &node);
			add_entry_to_dir(parent_path_inum, filename, child_inum);
		}
	}
	return child_inum;
}

uint64_t SBFS_mknod(char *path)
{
	char parent_path[5 * MAX_FILENAME];
	if (SBFS_namei(path) != 0)
	{
		return 0;
	}
	int len = strlen(path);
	int i = 0;
	while (*(path + len - 1) != '/')
	{
		len--;
	}
	memcpy(parent_path, path, len);
	if (len == 1)
		parent_path[len] = 0;
	else
		parent_path[len - 1] = 0;
	char *filename = path + len;
	printf("path %s filename %s\n", parent_path, filename);
	uint64_t parent_path_inum = SBFS_namei(parent_path);
	uint64_t child_inum;
	if (parent_path_inum == 0)
	{
		return 0;
	}
	else
	{
		inode parent_node;
		read_inode(parent_path_inum, &parent_node);
		if ( (parent_node.permission_bits & FILEMASK) >> 12 != DIR)
		{
			return 0;
		}
		else
		{
			inode node;
			child_inum = allocate_inode();
			read_inode(child_inum, &node);
            setFiletype(&node,NORMAL);
			write_inode(child_inum, &node);
			add_entry_to_dir(parent_path_inum, filename, child_inum);
		}
	}
	return child_inum;
}

//TODO: delete from parent dir

/**
 *  rmdir - remove empty directories
**/
int SBFS_rmdir(char *path) {
    inode node;
    char entry_name[MAX_FILENAME];
    dir entry;
    uint64_t parent_dir_inum = find_parent_dir_inum(path);
    if (parent_dir_inum == 0) {
        printf("\nlocate failed for path: %s\n", path);
        return -1;
    }
    get_entryname(path, entry_name);
    int index = find_file_entry(parent_dir_inum, entry_name, &entry);
    if (index == -1) {
        printf("\nlocate failed for path: %s\n", path);
        return -1;
    }
    printf("rmdir %s: inum %ld\n", path, entry.inum);

    read_inode(entry.inum, &node);
    if ((node.permission_bits & FILEMASK) >> 12 != DIR || node.size != 0) {
        printf("\npath: %s is not an empty directory.\n", path);
        return -1;
    }


    printf("rmdir %ld: inum %ld\n", parent_dir_inum, entry.inum);
    delete_entry_from_dir(parent_dir_inum, index);
    node.link -= 1;
    if (node.link == 0) {
        free_inode(entry.inum);
    } else {
        write_inode(entry.inum, &node);
    }
    return 0;
}

/******
** unlink - call the unlink function to remove the specified file
******/
int SBFS_unlink(char *path) {
    inode node;
    dir entry;
    char entry_name[MAX_FILENAME];
    uint64_t parent_path_inum = find_parent_dir_inum(path);
    if (parent_path_inum == 0)
        return 0;
    get_entryname(path, entry_name);
    int index = find_file_entry(parent_path_inum, entry_name, &entry);

    if (index == -1) {
        printf("\nlocate failed for path: %s\n", path);
        return -1;
    }

    printf("unlink %ld\n", entry.inum);
    read_inode(entry.inum, &node);

    if ((node.permission_bits & FILEMASK) >> 12 != NORMAL) {
        printf("\n%s is not a file\n", path);
        return -1;
    } else {
        printf("unlink parent inum %ld\n", parent_path_inum);
        delete_entry_from_dir(parent_path_inum, index);
        node.link -= 1;
        if (node.link == 0) {
            free_inode(entry.inum);
        } else {
            write_inode(entry.inum, &node);
        }
    }
    return 0;
}

int SBFS_close(int inum)
{
	return 0;
}

uint64_t SBFS_open(char *filename, int mode)
{
	uint64_t inum = SBFS_namei(filename);
	return inum;
}

void SBFS_init()
{
	mkfs();
	create_root_dir();
}

/**
 * when init is not set to 1
 * reset the with 1
 * and please make sure it is a dir, a link file should run read_symlink first
 **/
dir *SBFS_readdir(uint64_t inum,int init)
{

	static int i;
	static uint64_t present_inum = -1;
    static uint64_t identifier = 0;
	static int item_count;
	static inode node;
	static dir entry;
    if(init != 1) {
        if (identifier != inum) {
            i = 0;
            identifier = inum;
            present_inum = read_symlink(inum);
            read_inode(present_inum, &node);
            assert( (node.permission_bits & FILEMASK) >> 12 == DIR);
            item_count = node.size / sizeof(dir);
        }
        assert((node.size % sizeof(dir)) == 0);
        while (1) {
            if (i >= item_count)
                return NULL;
            SBFS_readdir_raw(present_inum, i, &entry);
            i += 1;
            if (entry.inum != 0) {
                return &entry;
            }
        }
    }
    else{
        i = 0;
        identifier = 0;
        present_inum = -1;
        item_count = 0;
        memset(&node,0,sizeof(node));
        memset(&entry,0,sizeof(entry));
    }
}


/**
 * 0 for failure
 * 1 for success
 * flags EXCHANGE or NOREPLACE
 **/
int SBFS_rename(char* path,char* newname,unsigned int flags){
    char old[MAX_FILENAME];
    char new[MAX_FILENAME];
    dir old_entry;
    dir new_entry;

    uint64_t inum_old_parent = find_parent_dir_inum(path);
    if(inum_old_parent == 0)
        return 0;
    uint64_t inum_new_parent = find_parent_dir_inum(newname);
    if(inum_new_parent == 0)
        return 0;

    get_entryname(path,old);
    get_entryname(newname,new);
    int new_index = find_file_entry(inum_new_parent,new,&new_entry);
    int old_index = find_file_entry(inum_old_parent,old,&old_entry);

    if(old_index == -1){
        printf("file doesn't exist in %s\n",path);
        return 0;
    }
    if(new_index == -1){
        if(inum_new_parent==inum_old_parent){
            strcpy(old_entry.filename,new);
            write_entry(inum_old_parent,old_index,&old_entry);
        }
        else{
            delete_entry_from_dir(inum_old_parent,old_index);
            add_entry_to_dir(inum_new_parent,new,old_entry.inum);
        }
    }
    else {
        if (flags == NOREPLACE) {
            printf("new file exist %s\n",newname);
            return 0;
        }
        else if (flags == EXCHANGE) {
            delete_entry_from_dir(inum_new_parent,new_index);
            delete_entry_from_dir(inum_old_parent,old_index);
            printf("old %s new %s\n",old_entry.filename,new_entry.filename);
            add_entry_to_dir(inum_old_parent,new_entry.filename,new_entry.inum);
            add_entry_to_dir(inum_new_parent,old_entry.filename,old_entry.inum);
        }
        else {
            printf("invalid flag\n");
            return 0;
        }
    }

    return 1;
}

int SBFS_truncate(char* path, uint64_t newsize){
    uint64_t inum = SBFS_namei(path);
    inode node;
    if(inum == 0){
        return 0;//not found
    }
    else {
        read_inode(inum, &node);
        printf("inum %ld size %ld %ld\n",inum,node.size,newsize);
        if(node.size < newsize)
            return 0;
        else{
            node.size = newsize;
            node.permission_bits = node.permission_bits & ( ~(0xffff & (USERBMASK | GROUPBMASK) ) );
            write_inode(inum,&node);
        }
    }
    return 1;
}

void create_root_dir() {
    inode root_node;
    uint64_t root = allocate_inode();
    assert(root == ROOT);
    read_inode(root, &root_node);
    setFiletype(&root_node, DIR);
    write_inode(root, &root_node);
}

/**
 * create a symlink file
 * checking cycle not implemented
 * **/
int SBFS_symlink(char* path,char* filename){
    uint64_t inum = SBFS_namei(path);
    dir entry;
    char entry_name[MAX_FILENAME];
    if(inum==0){
        printf("path %s not found\n",path);
        return 0;
    }
    get_entryname(filename,entry_name);
    uint64_t new_parent_inum = find_parent_dir_inum(filename);
    if(new_parent_inum == 0){
        printf("parent path %s not found",filename);
        return 0;
    }
    int index = find_file_entry(new_parent_inum,entry_name,&entry);
    if(index != -1){
        printf("new file %s already exists\n",entry_name);
        return 0;
    }
    inode node;
    uint64_t new_inum = allocate_inode();
    read_inode(inum,&node);
    assert(node.link==1);
    node.permission_bits &= ~FILEMASK;
    node.permission_bits |= (SYMBOLIC << 12) & FILEMASK;
    char buf[400];
    strcpy(buf,path);
    node.direct_blocks[0] = allocate_data_block();
    if(node.direct_blocks[0]==0)
        return 0;
    write_block(node.direct_blocks[0],0,strlen(buf),buf);
    add_entry_to_dir(new_parent_inum,entry_name,new_inum);
    write_inode(new_inum,&node);
    return 1;
}

/**
 * create a hard link
 * **/
int SBFS_link(char* path, char* newpath) {
    uint64_t dest_inum = SBFS_namei(path);
    inode node;
    if (dest_inum == 0)
        return 0;
    else{
        char new[MAX_FILENAME];
        read_inode(dest_inum,&node);
        node.link += 1;
        uint64_t inum_new_parent = find_parent_dir_inum(newpath);
        if(inum_new_parent == 0){
            printf("%s not exist\n",newpath);
            return 0;
        }
        get_entryname(newpath,new);
        add_entry_to_dir(inum_new_parent,new,dest_inum);
        write_inode(dest_inum,&node);
    }
    return 1;
}

int SBFS_chmod(char* filename,uint16_t mode){
    uint64_t inum = SBFS_namei(filename);
    inode node;
    if(inum==0)
        return 0;
    read_inode(inum,&node);
    node.permission_bits &= ~(WORLDMASK|OWNERMASK|GROUPMASK);
    int i = 0;
    uint16_t new_permit = 0;
    while(mode!=0){
        uint16_t permit_tmp = mode % 10;
        mode = mode / 10;
        new_permit |= permit_tmp << (i*3);
        i += 1;
    }
    node.permission_bits |= new_permit;
    node.last_access_time = node.last_modify_time = get_nanos();
    write_inode(inum,&node);
    return 1;
}
/**
 * tv[0] last access time
 * tv[1] last modify time
 **/
int SBFS_utime(char* path,const struct timespec tv[2]){
    uint64_t inum = SBFS_namei(path);
    if (inum == 0)
        return 0;
    inode node;
    read_inode(inum,&node);
    node.last_access_time = tv[0].tv_sec * 1000000000L + tv[0].tv_nsec;
    node.last_modify_time = tv[1].tv_sec * 1000000000L + tv[1].tv_nsec;
    write_inode(inum,&node);
    return 1;
}

int SBFS_chown(char* path,uint16_t uid,uint16_t gid) {
    uint64_t inum = SBFS_namei(path);
    if (inum == 0)
        return 0;
    inode node;
    read_inode(inum, &node);
    node.owner = uid;
    node.group = gid;
    node.permission_bits = node.permission_bits & (~(0xffff & (USERBMASK | GROUPBMASK)));
    write_inode(inum, &node);
    return 1;
}
/*
int main()
{
    char buf[] = "hello world";
    char read_buf[200];
	dir *entry;
	SBFS_init();
    uint64_t inum = SBFS_mkdir("/abc");
    inum = SBFS_mknod("/testtest1");
    inum = SBFS_mknod("/abc/testtest1");
    SBFS_symlink("/abc","/bcd");
    printf("read symlink %ld\n", read_symlink(5));
    SBFS_write(inum,0,sizeof(buf),buf);
    SBFS_truncate("/bcd/testtest1",4);
    SBFS_chmod("/bcd/testtest1",777);
    int res = SBFS_rename("/testtest1","/abc/testtest1",EXCHANGE);
    if(res == 0){
        printf("failed rename\n");
        return 0;
    }

    uint64_t read_size = SBFS_read(inum,0,BLOCKSIZE,read_buf);
    printf("%s\n",read_buf);

	while (entry = SBFS_readdir(ROOT,0)) {
		printf("readdir %s inum %ld\n", entry->filename, entry->inum);
	}
	return 0;
}
*/
/*TODO:

1. Create a symbolic link (passed)
symlink(path, link)
2. Rename a file (passed)
rename(path, newpath)
3. create a hard link (passed)
link(path, newpath)
4. change permission
chmod(path, mode)
5. change owner
chown(path, uid, gid)
6. change size of a file
truncate(path, newsize)
7. change access/ modification time
utime(path, ubuf)
8. read symbolic link file
readlink()
*/