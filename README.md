# CS270-operating-system

group work from team SBFS  

## Starting the file system

1. Requirement: **Fuse3** is required.

2. Download the repo:

   ```
   git clone git@github.com:ViViVidam/CS270-operating-system.git
   ```

      or

   ```
   wget https://github.com/ViViVidam/CS270-operating-system/archive/main.tar.gz
   tar zxvf CS270-operating-system-main.tar.gz
   ```
3. Environment prepare  

   To run this on Centos 7  
   
   please run
   
   ```
   (sudo) yum -y install gcc make git fuse3 fuse3-devel man-pages gdb
   (sudo) yum update -y
   (sudo) yum install -y centos-release-scl
   (sudo) yum install -y devtoolset-9
   scl enable devtoolset-9 bash
   ```
   
   this will install gcc-9 which is need to compile fuse3, and add it to the environment variable  
   
   if you run `scl enable devtoolset-9 bash` as root, then gcc version is 9 when you make the project as root  
   
   so make sure you `scl enable devtoolset-9 bash` and make the project both as root or user  
   
4. Setup the file system

   start the file system with

   ```
   cd CS270-operating-system-main
   make run
   ```

   The mount point will be created at `./mount/`. And a `hello` file will be automatically created in the file system.

## Test

- for normal case: `cd` to the mount point

- for debug case: start a new terminal and  `cd` to the mount point  

  ```
  cd ./mount
  ```

- system call functions our current implementation suppots:

  -  `read` `write` `mkdir` `rmdir` `unlink` `close` `namei` `readdir` `mknod`  
  - you can also find them in `SBFS.h`

- we have tested the linux commands below:

|    command    |        purpose        |
| :-----------: | :-------------------: |
|      ls       |  read directory file  |
| echo with `>` |    test write file    |
|     touch     |      create file      |
|      cat      |       read file       |
|     mkdir     | write directory file  |
|     touch     |     create a file     |
|      cd       |     for integrity     |
|      rm       | test unlink and rmdir |
| test program  | a further validation  |

You can play with these commands in a new terminal and check the log in the original terminal.

- Also, we provide a test C program (`test.c`) including some basic file operations, with which you can create a new file named `a`  and write some characters into it: 
  - To run the test program, type `./test string offset` in the root directory (first `cd ..` if you are within the mount point)
  - `string` is what you want to write into the file and `offset` is the input of the `lseek`  
  - you will find the new file named `a` after `cd` into the mount point, use `cat` to check what you have written 

- We guarantee no robustness, so the file system may fail if you are trying to lseek the entire file size or deleting file that doesn't exist.

## End testing

```
make clean
```

## Reference

1. [Writing a FUSE Filesystem: a Tutorial](https://www.cs.nmsu.edu/~pfeiffer/fuse-tutorial/)


