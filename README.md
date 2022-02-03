# CS270-operating-system

group work from team SBFS  

## Starting the file system

1. Requirement: Fuse3 is required.

2. Download the repo:

   ```
   git clone git@github.com:ViViVidam/CS270-operating-system.git
   ```

      or

   ```
   wget https://github.com/ViViVidam/CS270-operating-system/archive/main.tar.gz
   tar zxvf CS270-operating-system-main.tar.gz
   ```

3. Setup the file system
   there are two modes to run the file system, normal and debug
   
###normal case

   start the file system with
   ```
   cd CS270-operating-system-main
   make run
   ``` 
   this will create a mount point at `./mount/`, also copy a helloworld file into the file system
      
###debug case

   the debug mode, where you can see the output from the file system:
   ```
   cd CS270-operating-system-main
   make debug
   ```
   you can see the output from the file system in this case


##test

   for normal case: `cd` to the mount point and enjoy  
   for debug case you have to start a new terminal then `cd` to the mount point  
   
   the functions has been implemented are:
   `read` `write``mkdir``rmdir``unlink``close``namei``readdir``mknod`  
   
you can also find it in `SBFS.h`

   we have tested several linux command:

| command line  |        purpose        |
|:-------------:|:---------------------:|
|      ls       |  read directory file  |
| echo with `>` |    test write file    |
|     touch     |      create file      |
|      cat      |       read file       |
|     mkdir     | write directory file  |
|     touch     |     create a file     |
|      cd       |     for integrity     |
|      rm       | test unlink and rmdir |
| test program  | a further validation  |

feel free to play with these command lines after cd into the mount point   
   ```
   cd ./mount
   ```

To run the test program type `./test string offset` in the root directory  
`string` is what you want to write into the file and `offset` is the input of the `lseek`  
also you will be able to test a file name `a` after cd into the mount point  

we guarantee no robustness, so file system may fail if you are trying to lseek the entire file size or deleting file that doesn't exist

## End testing
   ```
   make clean
   ```

## Reference

1. [Writing a FUSE Filesystem: a Tutorial](https://www.cs.nmsu.edu/~pfeiffer/fuse-tutorial/)

