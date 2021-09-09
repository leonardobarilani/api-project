# Hierarchical File System Project

This repository contains the final project of my Algorithms and principles of computer science (Algoritmi e Principi dell'Informatica, aka A.P.I.) course. The goal of the project is to implement a simple virtual filesystem API using C99 and the standard libc.

## Building

    $ git clone https://github.com/leonardobarilani/api-project.git
    $ cd api-project
    $ ./compile.sh

## Testing

    $ ./run_tests.sh

Output `All tests passed` on success, otherwise the `diff` between the output of the failed test and the expected output.

## Assignement

The original assignement was provided in italian. Here is a shortened translation of it in english:

### Summary

The goal of the project is to implement a simple hierarchical filesystem that can contain folders and files identified both by names. The program will receive a list of commands to be executed on the fylesystem from `stdin` and it must output the results in `stdout`. The project has to be implemented in standard C (C99) using only the standard library `libc`.

### Input format

Paths are represented in the usual UNIX format, separated with `/`.

The following costraints hold:

* Names are alfanumeric and are 255 characters long
* The maximum depth for the tree is 255
* The maximum number of children is 1024

The program must implement the following commands:

* `create <path>`: create an empty file. Output `ok` on success, `no` on failure
* `create_dir <path>`: create an empty directory. Output `ok` on success, `no` on failure
* `read <path>`: read a file. Output `contenuto ` followed by the content of the file on success, `no` on failure
* `write <path> <content>`: write in an existing file, overwriting the previous content. `<content>` is a string delimited by `"` containing letters and spaces. Output `ok` on success, `no` on failure
* `delete <path>`: delete a resource, if it has no children. Output `ok` on success, `no` on failure
* `delete_r <path>`: delete a resource and all his descendants. Output `ok` on success, `no` on failure
* `find <name>`: find all the resources with the specified name. Output `ok` if at least one resource is found followed by the paths of the resources found with that name in lexicographical order (the whole path has to be considered in the ordering, including `/`), `no` if no resource is found
* `exit`: close the program

### Time Complexity Requirements

* `l`: length of the path
* `d`: number of resources in the whole filesystem
* `d_path`: number of children resources in the path
* `f`: number of resources found in the research

|Command        |Complexity             |
|---------------|-----------------------|
|`create`       |O(`l`)                 |
|`create_dir`   |O(`l`)                 |
|`read`         |O(`l+\|file content\|`)|
|`write`        |O(`l+\|file content\|`)|
|`delete`       |O(`l`)                 |
|`delete_r`     |O(`d_path`)            |
|`find`         |O(`d+f^2`)             |

## Implementation

The implementation of the filesystem uses two data structures.

    typedef struct node_s {
        char type;			// FILE | DIR
        char tombstone;		// ALIVE | DEAD
        char * name;		
        char * data;		// Used only if type == FILE

        struct node_s * parent;				// Parent node
        struct node_s * nodes[MAX_DIRS];	// Children nodes
    } node;

`node` is a structure used to build the tree that stores both files and directories. `type` specifies if the node is either a directory or a file. `tombstone` specifies if the node contains valid informations or if it is not used. `data` contain the files content and of course it is used only if the node is a file node. `nodes` is a hash table (indexed using the Fibonacci hash function) and contain the pointers to the children of the node and it is of course used only if the node is a directory. Collision in the hash table are solved using linear probing. `parent` is a pointer to the aprent folder of the node.

    typedef struct bt_s {
        char * key;

        struct bt_s * l;
        struct bt_s * r;
    } bt;

`bt` is a helper structure only used by the `find` command. It is a binary ordered tree, first filled with the paths and then printed in-order as needed by the `find`. It is thus created and deleted for every command call.
