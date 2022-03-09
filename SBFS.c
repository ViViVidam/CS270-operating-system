#include "SBFS.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include "SBFSHelper.h"
#include <fcntl.h>

#define MIN(a, b) ((a > b) ? b : a)
#define MAX(a, b) ((a > b) ? a : b)

int SBFS_getattr(char* path, struct stat* file_state){
    printf("111111111\n");
    uint64_t inum = SBFS_namei(path);
    if(inum==0) {
        printf("inum not found %s\n",path);
        return 0;
    }
    inode node;
    read_inode(inum,&node);
    file_state->st_gid = node.group;
    file_state->st_uid = node.owner;
    file_state->st_size = node.size;
    file_state->st_nlink = node.link;
    file_state->st_mode = node.permission_bits;
    if(ISDIR(node.permission_bits)){
        file_state->st_mode = S_IFDIR;
    }
    else if(ISSYM(node.permission_bits)){
        file_state->st_mode = S_IFLNK;
    }
    else if(ISFILE(node.permission_bits)){
        file_state->st_mode = S_IFREG;
    }
    else {
        printf("unrecongnizede file type %x\n",node.permission_bits);
        return 0;
    }
    uint16_t stick_user_group = (node.permission_bits & (USERBMASK|GROUPBMASK|STICKBIT)) << 3;
    uint16_t user = (node.permission_bits&OWNERMASK)<<2;
    uint16_t group = (node.permission_bits&GROUPMASK) << 1;
    uint16_t world = (node.permission_bits&WORLDMASK);
    //file_state->st_mode = file_state->st_mode|(stick_user_group|user|group|world);
    file_state->st_mode |= (node.permission_bits&(USERBMASK|GROUPBMASK|STICKBIT|OWNERMASK|GROUPMASK|WORLDMASK));
    printf("\n\n%x %x\n\n",file_state->st_mode,node.permission_bits);
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
int SBFS_readlink(char* path, char* buf, unsigned long size) {
    uint64_t inum = SBFS_namei(path);
    if (inum == 0)
        return -1;
    inode node;
    read_inode(inum, &node);
    if ((node.permission_bits & FILEMASK) >> 12 != SYMBOLIC) {
        return -1;
    }
    if(size < node.size) {
        read_block_cache(node.direct_blocks[0], 0, size, buf);
        buf[size-1] = 0;
    }
    else
        read_block_cache(node.direct_blocks[0], 0, node.size, buf);
    return 0;
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
        while (*pointer != '/' && *pointer != 0 && i<= MAX_FILENAME) {
            filename[i++] = *pointer;
            pointer += 1;
        }
        if(*pointer != '/' && *pointer != 0)
            return 0;
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

int SBFS_read(uint64_t inum, uint64_t offset, int64_t size, void *buf)
{
	char *buffer = (char *)buf;
	inode node;
    
	read_inode(inum, &node);
    if(node.size<offset){
        strcpy(buf,"");
        return 0;
    }
	int read_size, pre_size;
    read_size = pre_size = MIN(size,node.size-offset);

    //printf("read block inum %ld offset %ld size %ld\n",inum,offset,size);
    int block_index = offset / BLOCKSIZE;
	uint64_t read_block_id = block_id_helper(&node, block_index, H_READ);
	uint64_t block_offset = offset % BLOCKSIZE;
	assert(block_offset < BLOCKSIZE);

	int read_bytes = read_block_cache(read_block_id, block_offset, read_size, buf);
	read_size -= read_bytes;
	buffer += read_bytes;
    //printf("read bytes %d size %ld\n",read_bytes,read_size);

	while (read_size > 0)
	{
		block_index += 1;
		uint64_t block_id = block_id_helper(&node, block_index, H_READ);
		assert(block_id != 0);
		read_bytes = read_block_cache(block_id, block_offset, size, buf);
		buffer += read_bytes;
		read_size -= read_bytes;
        //printf("read bytes %d size %ld\n",read_bytes,read_size);
	}

    node.last_access_time = get_nanos();
    write_inode(inum,&node);
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
    int block_index = offset / BLOCKSIZE;
	int write_block_id = block_id_helper(&node, offset / BLOCKSIZE, H_CREATE);
	uint64_t block_offset = offset % BLOCKSIZE;
	assert(block_offset < BLOCKSIZE);

	int write_bytes = write_block_cache(write_block_id, block_offset, size, buf);
	size -= write_bytes;
	buffer += write_bytes;
    //printf("write bytes %d size %ld\n",write_bytes,size);
	while (size > 0)
	{
		block_index += 1;
        //printf("start %d\n",block_index);
		write_block_id = block_id_helper(&node, block_index, H_CREATE);
		write_bytes = write_block_cache(write_block_id, block_offset, size, buf);
		buffer += write_bytes;
		size -= write_bytes;
	}

	//node.size = MIN(node.size, upperbound);
    node.size = upperbound;
    node.last_access_time = node.last_modify_time = get_nanos();

	write_inode(inum, &node);
	return size;
}

/* direcotry is dir, the item is empty when inode = 0 */
/*
return 0: can not mkdir
*/
uint64_t SBFS_mkdir(char *path,unsigned int userId,unsigned int groupId)
{
	char parent_path[MAX_PATH];
	if (SBFS_namei(path) != 0)
	{
		return 0;
	}
	int len = strlen(path);
	if(len>MAX_PATH)
        return 0;
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
    if(strlen(filename) > MAX_FILENAME)
        return 0;
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
            node.owner = userId;
            node.group = groupId;
            node.permission_bits |= ((GROUPMASK|OWNERMASK|WORLDMASK)&0x1FD);
			node.size = 0;
            node.last_access_time = node.last_modify_time = get_nanos();
			write_inode(child_inum, &node);

			add_entry_to_dir(parent_path_inum, filename, child_inum);
		}
	}
	return child_inum;
}

uint64_t SBFS_mknod(char *path,unsigned int userId,unsigned int groupId)
{
	char parent_path[MAX_PATH];
	if (SBFS_namei(path) != 0)
	{
		return 0;
	}
	int len = strlen(path);
	if(len>MAX_PATH)
        return 0;
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
    if(strlen(filename) > MAX_FILENAME)
        return 0;
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
            node.owner = userId;
            node.group = groupId;
            node.permission_bits |= ((GROUPMASK|OWNERMASK|WORLDMASK)&0x1b4);
            //rw_rw_r__
            node.last_access_time = node.last_modify_time = get_nanos();
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
    if(get_entryname(path, entry_name)==0)
        return -1;
    int index = find_file_entry(parent_dir_inum, entry_name, &entry);
    if (index == -1) {
        printf("\nlocate failed for path: %s\n", path);
        return -1;
    }
    printf("rmdir %s: inum %ld\n", path, entry.inum);

    read_inode(entry.inum, &node);
    if ((node.permission_bits & FILEMASK) >> 12 != DIR) {
        printf("\npath: %s is not a directory.\n", path);
        return -1;
    }
    dir tmp;
    for (int k = 0; k < (node.size / sizeof(entry)); k++) {
        SBFS_readdir_raw(entry.inum, k, &tmp);
        if (tmp.inum != 0) {
            printf("\npath: %s is not an empty directory.\n", path);
            return -1;
        }
    }

    printf("rmdir %ld: inum %ld\n", parent_dir_inum, entry.inum);
    delete_entry_from_dir(parent_dir_inum, index);
    node.link -= 1;
    if (node.link == 0) {
        printf("free inode\n");
        free_inode(entry.inum);
    } else {
        node.last_access_time = node.last_modify_time = get_nanos();
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
    read_inode(entry.inum, &node);

    if ((node.permission_bits & FILEMASK) >> 12 != NORMAL && (node.permission_bits & FILEMASK) >> 12 != SYMBOLIC) {
        printf("\n%s is not a file\n", path);
        return -1;
    } else {
        printf("unlink parent inum %ld\n", parent_path_inum);
        delete_entry_from_dir(parent_path_inum, index);
        node.link -= 1;
        if (node.link == 0) {
            free_inode(entry.inum);
        } else {
            node.last_access_time = node.last_modify_time = get_nanos();
            write_inode(entry.inum, &node);
        }
    }
    return 0;
}

int SBFS_close(int inum)
{
	return 0;
}

uint64_t SBFS_open(char *filename, unsigned int userId,unsigned int groupId,unsigned int flag) {
    uint64_t inum = SBFS_namei(filename);
    inode node;
    read_inode(inum, &node);
    uint16_t access_mode = flag & O_ACCMODE;
    if (access_mode == O_WRONLY) {
        if(checkWrite(&node,userId,groupId))
            return inum;
        return 0;
    } else if (access_mode == O_RDONLY) {
        if(checkRead(&node,userId,groupId))
            return inum;
        return 0;
    } else if (access_mode == O_RDWR) {
        if(checkRead(&node,userId,groupId) && checkWrite(&node,userId,groupId))
            return inum;
        return 0;
    }
    printf("unrecognized flag %d %x\n",flag,flag);
    return 0;
}

void SBFS_init(unsigned int uid,unsigned int gid){
    if(mkfs()==0)
	    create_root_dir(uid,gid);
    int res = getMountPoint(mountpoint,256);
    assert(res!=0);
    printf("mountpoint %s\n",mountpoint);
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
        printf("inode size in dir %d %d\n",node.size,present_inum);
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
    if(get_entryname(newname,new)==0)
        return 0;

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
            add_entry_to_dir(inum_old_parent,old_entry.filename,new_entry.inum);
            add_entry_to_dir(inum_new_parent,new_entry.filename,old_entry.inum);
        }
        else {
            delete_entry_from_dir(inum_old_parent,old_index);
            delete_entry_from_dir(inum_new_parent,new_index);
            add_entry_to_dir(inum_new_parent,new_entry.filename,old_entry.inum);
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
            node.last_access_time = node.last_modify_time = get_nanos();
            write_inode(inum,&node);
        }
    }
    return 1;
}

void create_root_dir(unsigned int uid,unsigned int gid) {
    inode root_node;
    uint64_t root = allocate_inode();
    assert(root == ROOT);
    read_inode(root, &root_node);
    setFiletype(&root_node, DIR);
    root_node.owner = uid;
    root_node.group = gid;
    root_node.permission_bits |= ((GROUPMASK|OWNERMASK|WORLDMASK)&0x1FD);
    //rwxrwxr_x
    root_node.size = 0;
    write_inode(root, &root_node);
}

/**
 * create a symlink file
 * checking cycle not implemented
 * **/
int SBFS_symlink(char* path,char* filename) {
    dir entry;
    char entry_name[MAX_FILENAME];
    printf("SBFS_symlink %s\n",path);
    if(get_entryname(filename, entry_name)==0)
        return 0;
    uint64_t new_parent_inum = find_parent_dir_inum(filename);
    if (new_parent_inum == 0) {
        printf("parent path %s not found", filename);
        return 0;
    }
    int index = find_file_entry(new_parent_inum, entry_name, &entry);
    if (index != -1) {
        printf("new file %s already exists\n", entry_name);
        return 0;
    }
    inode node;
    uint64_t new_inum = allocate_inode();
    read_inode(new_inum, &node);
    assert(node.link > 0);
    setFiletype(&node,SYMBOLIC);
    char buf[400];
    strcpy(buf, path);
    node.direct_blocks[0] = allocate_data_block();
    if (node.direct_blocks[0] == 0)
        return 0;
    write_block_cache(node.direct_blocks[0], 0, strlen(buf), buf);
    node.size = strlen(buf);
    node.permission_bits |= ((GROUPMASK|OWNERMASK|WORLDMASK)&0x1FF);
    add_entry_to_dir(new_parent_inum, entry_name, new_inum);
    node.last_access_time = node.last_modify_time = get_nanos();
    write_inode(new_inum, &node);
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
    node.permission_bits = 0;

    if(S_ISLNK(mode))
        setFiletype(&node,SYMBOLIC);
    else if(S_ISREG(mode))
        setFiletype(&node,NORMAL);
    else if(S_ISDIR(mode))
        setFiletype(&node,DIR);
    else {
        printf("unsupported file type %x",mode);
        return 0;
    }
    mode &= (USERBMASK|GROUPBMASK|STICKBIT|WORLDMASK|OWNERMASK|GROUPMASK);
    node.permission_bits |= mode;
    node.permission_bits |= 0x8000;
    node.last_access_time = node.last_modify_time = get_nanos();
    write_inode(inum,&node);
    return 1;
}
/**
 * tv[0] last access time
 * tv[1] last modify time
 **/
int SBFS_utime(char* path, const struct timespec tv[2]){
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
    node.last_access_time = node.last_modify_time = get_nanos();
    write_inode(inum, &node);
    return 1;
}

uint64_t SBFS_opendir(const char* path,unsigned int userId,unsigned int groupId){
    uint64_t inum = SBFS_namei(path);
    if (inum == 0)
    {
        printf("open dir failed.\n");
        return 0;
    }
    inode node;
    read_inode(inum,&node);
    if(checkRead(&node,userId,groupId))
        return inum;
    return 0;
}

void SBFS_flush_all(){
    cache_flush_all();
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
9.flush()
10.permission checking
11.relevant 2 absolute path convert
*/
