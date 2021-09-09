/*
	Copyright 2017 Leonardo Barilani

	Licensed under the Apache License, Version 2.0 (the "License");
	you may not use this file except in compliance with the License.
	You may obtain a copy of the License at

		http://www.apache.org/licenses/LICENSE-2.0

	Unless required by applicable law or agreed to in writing, software
	distributed under the License is distributed on an "AS IS" BASIS,
	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
	See the License for the specific language governing permissions and
	limitations under the License.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Type used in the node_s struct to mark the node as a file or a directory
#define FILE 1
#define DIR 0

// Tombstone used in the node_s struct to mark the node as deleted
#define ALIVE 1
#define DEAD 0

// Maximum number of directories in a node_s
#define MAX_DIRS 1024
// Maximum number of characters for a folder/file name
#define MAX_NAME 255

// Constant used in the Fibonacci hashing function
const unsigned int HASH_CONST = (unsigned int)(0.618033989 * 1024.0);

/*
	Tree with MAX_DIRS children indexed using the Fibonacci hash function
	Used for storing data files
*/
typedef struct node_s {
	char type;			// FILE | DIR
	char tombstone;		// ALIVE | DEAD
	char * name;		
	char * data;		// Used only if type == FILE

	struct node_s * parent;				// Parent node
	struct node_s * nodes[MAX_DIRS];	// Children nodes
} node;

/*
	Binary tree used only by the find command
*/
typedef struct bt_s {
	char * key;

	struct bt_s * l;
	struct bt_s * r;
} bt;

char * my_getline();
char * my_strdup(char * str);

node * init_node(char type, char * name);
node * enter_node(node * cur, char * directory);

void create(char type, char * parameters);
void write(char * parameters, char * data);
void read(char * parameters);
void delete(char * parameters);
void delete_r(char * parameters);
void find(char * parameters);

/*
	Stores the entire filesystem
*/
node * root;

/*
	Temporarily stores the results of the find command
	(first we will the tree with the found paths and 
	then we print it in-order)
*/
bt * bt_root;

int main(int argc, char *argv[])
{
	int exit;
	char * str;
	char * command;
	char * parameters;
	char * data;

	if (argc == 0)
		return 1;

	root = init_node(DIR, "");

	exit = 0;
	while(!exit)
	{
		str 		= my_getline();
		command 	= strtok(str, " \t\r");
		parameters 	= strtok(NULL, " \t\r");

		if(strcmp(command, "exit") == 0)
			exit = 1;
		else if(strcmp(command, "create"	) == 0)
			create(FILE, parameters);
		else if(strcmp(command, "create_dir") == 0)
			create(DIR, parameters);
		else if(strcmp(command, "read"		) == 0)
			read(parameters);
		else if(strcmp(command, "write"		) == 0)
		{
			data = my_strdup(strtok(NULL, "\""));
			data[strlen(data)] = '\0';
			write(parameters, data);
			/*
				the free in the my_strdup() is called in delete,
				delete_r or in the write in the case we are 
				overwriting something
			*/
		}
		else if(strcmp(command, "delete"	) == 0)
			delete(parameters);
		else if(strcmp(command, "delete_r"	) == 0)
			delete_r(parameters);
		else if(strcmp(command,  "find"		) == 0)
			find(parameters);

		free(str);
	}

	return 0;
}

/*
	Implementation of the function getline() since
	it is not present in the C99 standard
*/
inline char * my_getline()
{
	char *buffer;
	int l;
	int max_l;
	char ch;

	max_l 	= 256;
	buffer 	= malloc(sizeof(char) * max_l);

	ch 		= fgetc(stdin);
	for(l = 0; ch != EOF && ch != '\n' && ch != '\r' && ch != '\t'; l++)
	{
        if(l == max_l)
        {
        	max_l += 256;
            buffer = realloc(buffer, sizeof(char) * max_l);
        }
		buffer[l] 	= ch;
		ch 			= fgetc(stdin);
	}
	buffer[l] = '\0';

	return realloc(buffer, sizeof(char) * (l + 1));
}

/*
	Implementation of the function strdup() since
	it is not present in the C99 standard
*/
inline char * my_strdup(char * str)
{
	char * buf = NULL;
	if(str != NULL)
	{
		buf = malloc(sizeof(char) * (strlen(str) + 1));
		strncpy(buf, str, strlen(str) + 1);
	}
	return buf;
}

/*
	Fibonacci Hash Function (https://en.wikipedia.org/wiki/Hash_function#Fibonacci_hashing)
*/
int hash(char * str)
{
	int c;
	int hash = 0;

    while ((c = *str++))
        hash += c;

    return (int)(hash * HASH_CONST % MAX_DIRS);
}

/*
	Init a node and return a pointer to the allocated node
*/
node * init_node(char type, char * name)
{
	node * tmp = NULL;
	int i;

	if((tmp = (node*)malloc(sizeof(node))))
	{
		tmp->type 		= type;
		tmp->tombstone 	= ALIVE;
		tmp->name 		= my_strdup(name);
		if(type == DIR)
			tmp->data = NULL;
		else
			tmp->data = my_strdup("");
		tmp->parent = NULL;
		for(i = 0;i < MAX_DIRS;i++)
			tmp->nodes[i] = NULL;
	}
	return tmp;
}

/*
	Given a node and the name of a directory/file:
	Return the index of nodes that is free and 
	thus can contain a new directory/file
	Return -1 if there is no such node
*/
int pick_free_index(node * cur, char * directory)
{
	int beginning;
	int i;

	beginning = hash(directory);

	for(i = beginning;i < MAX_DIRS;i++)
		if(	cur->nodes[i] == NULL || 
			(cur->nodes[i] != NULL && 
			cur->nodes[i]->tombstone == DEAD))
			return i;

	for(i = 0;i < beginning;i++)
		if(	cur->nodes[i] == NULL || 
			(cur->nodes[i] != NULL && 
			cur->nodes[i]->tombstone == DEAD))
			return i;

	return -1;
}

/*
	Given a node and the name of a directory/file:
	Return the pointer of nodes that contains that directory
	Return NULL if there is no such node
*/
node * enter_node(node * cur, char * directory)
{
	int beginning;
	int i;

	beginning = hash(directory);

	for(i = beginning;i < MAX_DIRS;i++)
		if( cur->nodes[i] 		!= NULL && 
			cur->nodes[i]->tombstone == ALIVE &&
			cur->nodes[i]->type == DIR &&
			strcmp(cur->nodes[i]->name, directory) == 0)
			return cur->nodes[i];
		else if(cur->nodes[i] == NULL)
			return NULL;

	for(i = 0;i < beginning;i++)
		if( cur->nodes[i] 		!= NULL && 
			cur->nodes[i]->tombstone == ALIVE &&
			cur->nodes[i]->type == DIR &&
			strcmp(cur->nodes[i]->name, directory) == 0)
			return cur->nodes[i];
		else if(cur->nodes[i] == NULL)
			return NULL;

	return NULL;
}

/*
	Given a node and the name of a file:
	Return the index of nodes that contains that file
	Return -1 if there is no such node
*/
int pick_file(node * cur, char * directory)
{
	int beginning;
	int i;

	beginning = hash(directory);

	for(i = beginning;i < MAX_DIRS;i++)
		if( cur->nodes[i] 		!= NULL && 
			cur->nodes[i]->tombstone == ALIVE &&
			cur->nodes[i]->type == FILE &&
			strcmp(cur->nodes[i]->name, directory) == 0)
			return i;
		else if(cur->nodes[i] == NULL)
			return -1;

	for(i = 0;i < beginning;i++)
		if( cur->nodes[i] 		!= NULL && 
			cur->nodes[i]->tombstone == ALIVE &&
			cur->nodes[i]->type == FILE &&
			strcmp(cur->nodes[i]->name, directory) == 0)
			return i;
		else if(cur->nodes[i] == NULL)
			return -1;

	return -1;
}

/*
	Given a node and the name of a node:
	Return the index of nodes that containslinux get line
	the node with the specified name
	Return -1 if there is no such node
*/
int pick_node(node * cur, char * directory)
{
	int beginning;
	int i;

	beginning = hash(directory);

	for(i = beginning;i < MAX_DIRS;i++)
		if( cur->nodes[i] 		!= NULL && 
			cur->nodes[i]->tombstone == ALIVE &&
			strcmp(cur->nodes[i]->name, directory) == 0)
			return i;
		else if(cur->nodes[i] == NULL)
			return -1;

	for(i = 0;i < beginning;i++)
		if( cur->nodes[i] 		!= NULL && 
			cur->nodes[i]->tombstone == ALIVE &&
			strcmp(cur->nodes[i]->name, directory) == 0)
			return i;
		else if(cur->nodes[i] == NULL)
			return -1;

	return -1;
}

/*
	Iteratively access nodes in the filesystem to reach
	the resource path specified in the parameters.
	Return the node or NULL if no resource was reached
*/
node * reach_node(char * parameters)
{
	int done;		// 1 = terminate the iterative search
	int success;	// 1 = we found the resource
	node * cur;	
	char * target;	// name of the node to be searched in the current node

	done 	= 0;
	success = 0;
	cur 	= root;
	target 	= strtok(parameters, "/");

	while(!done)
	{
		// we didn't reached the directory of the resource

		if(target == NULL && cur != NULL)
		{
			// we totally consumed the path and we are in a node: success
			success = 1;
			done 	= 1;
		}
		else if(cur == NULL)
		{
			// we didn't consume the path but are not in a node anymore: unsuccess
			done 	= 1;
		}
		else
		{
			// access the current node and set the next directory to be found
			cur 	= enter_node(cur, target);
			target 	= strtok(NULL, "/");
		}
	}

	if(!success)
		// return NULL if I didn't find the specified resource
		cur = NULL;

	return cur;
}

/*
	Create a directory or a file
*/
void create(char type, char * parameters)
{
	node * cur;	
	char * name;	// name of the resource to write
	char * last;	// position of the resource to write
	int i;			
	int success;

	success = 0;

	last	= strrchr(parameters, '/');
	name 	= my_strdup(last + 1);
	*last	= '\0';

	if((cur = reach_node(parameters)))
	{
		if(type == DIR)
		{
			if(enter_node(cur, name) == NULL)
			{
				// the directory doesn't exist, I can create it

				if((i = pick_free_index(cur, name)) != -1)
				{
					cur->nodes[i] 		= init_node(type, name);
					cur->nodes[i]->parent = cur;
					success = 1;
				}
			}
		}
		else if(type == FILE)
		{
			if(pick_file(cur, name) == -1)
			{
				// the file doesn't exist, I can create it

				if((i = pick_free_index(cur, name)) != -1)
				{
					cur->nodes[i] 		= init_node(type, name);
					cur->nodes[i]->parent = cur;

					success = 1;
				}
			}
		}
	}
	
	if(success)
		printf("ok\n");
	else
		printf("no\n");

	free(name);
}

/*
	Write the data into file
*/
void write(char * parameters, char * data)
{
	node * cur;	
	char * name;	// name of the resource to write
	char * last;	// position of the resource to write
	int i;			
	int success;

	success = 0;

	last	= strrchr(parameters, '/');
	name 	= my_strdup(last + 1);
	*last	= '\0';

	if((cur = reach_node(parameters)))
	{
		if((i = pick_file(cur, name)) != -1)
		{
			// I've found the file, I can write the data

			if(cur->nodes[i]->data != NULL)
				free(cur->nodes[i]->data);
			cur->nodes[i]->data = data;
			success = 1;
		}
	}

	if(success)
		printf("ok %d\n", (int) strlen(data));
	else
		printf("no\n");

	free(name);
}

/*
	Read the content of the specified file
*/
void read(char * parameters)
{
	node * cur;	
	char * name;	// name of the resource to be read
	char * last;	// position of the resource to be read
	int i;			
	int success;

	success = 0;

	last	= strrchr(parameters, '/');
	name 	= my_strdup(last + 1);
	*last	= '\0';

	if((cur = reach_node(parameters)))
	{
		if((i = pick_file(cur, name)) != -1)
		{
			if(cur->nodes[i]->data != NULL)
			{
				printf("contenuto %s\n", cur->nodes[i]->data);
				success = 1;
			}
		}
	}

	if(!success)
		printf("no\n");
	free(name);
}

/*
	Delete resource (either file or empty directory)
*/
void delete(char * parameters)
{
	node * cur;	
	char * name;	// name of the resource to be deleted
	char * last;	// position of the resource to be deleted
	int i, j;			
	int success;
	int tombstone;

	success = 0;

	last	= strrchr(parameters, '/');
	name 	= my_strdup(last + 1);
	*last	= '\0';

	if((cur = reach_node(parameters)))
	{
		if((i = pick_node(cur, name)) != -1)
		{
			if(cur->nodes[i]->type == FILE)
			{
				free(cur->nodes[i]->name);
				free(cur->nodes[i]->data);
				cur->nodes[i]->tombstone = DEAD;
				success = 1;
			}
			else
			{
				tombstone = 0;
				success = 1;
				for(j = 0;j < MAX_DIRS && success;j++)
					if(cur->nodes[i]->nodes[j] != NULL)
					{
						if(cur->nodes[i]->nodes[j]->tombstone == ALIVE)
							// can't delete a directory that is not empty
							success = 0;
						else
							tombstone++;
					}

				if(success)
				{
					for(j = 0;tombstone && j < MAX_DIRS;j++)
						if(cur->nodes[i]->nodes[j] != NULL)
						{
							free(cur->nodes[i]->nodes[j]->name);
							free(cur->nodes[i]->nodes[j]->data);
							free(cur->nodes[i]->nodes[j]);
							tombstone--;
						}

					free(cur->nodes[i]->name);
					free(cur->nodes[i]->data);
					cur->nodes[i]->tombstone = DEAD;
				}
			}
		}
	}

	if(success)
		printf("ok\n");
	else
		printf("no\n");
	free(name);
}

/*
	Recursively delete the resources starting from cur node
*/
void delete_recursive(node * cur)
{
	int i;

	for(i = 0;i < MAX_DIRS;i++)
		if(cur->nodes[i] != NULL)
		{
			if(cur->nodes[i]->type == DIR)
				delete_recursive(cur->nodes[i]);
			else
				free(cur->nodes[i]->data);
			free(cur->nodes[i]->name);
			free(cur->nodes[i]);
		}
}

/*
	Delete resource recoursively
*/
void delete_r(char * parameters)
{
	node * cur;	
	char * name;	// name of the resource to be deleted
	char * last;	// position of the resource to be deleted
	int i;			
	int success;

	success = 0;

	last	= strrchr(parameters, '/');
	name 	= my_strdup(last + 1);
	*last	= '\0';

	if((cur = reach_node(parameters)))
	{
		if((i = pick_node(cur, name)) != -1)
		{
			// resource found, I can delete it
			success = 1;

			if(cur->nodes[i]->data != NULL)
				// if it is a file, delete the content
				free(cur->nodes[i]->data);
			else
				// if it is a directory, delete it recursively
				delete_recursive(cur->nodes[i]);
			free(cur->nodes[i]->name);
			free(cur->nodes[i]);
			cur->nodes[i] = NULL;
		}
	}

	if(success)
		printf("ok\n");
	else
		printf("no\n");
	free(name);
}

/*
	Init a bt node with the key
*/
bt * init_bt(char * key)
{
	bt * new = NULL;
	if((new = (bt*)malloc(sizeof(bt))))
	{
		new->key = key;
		new->l = NULL;
		new->r = NULL;
	}
	return new;
}
/*
	Insert the new bt in the tree in the correct position
*/
void insert_bt(bt * new)
{
	bt * pre = NULL;
	bt * cur = bt_root;

	while(cur != NULL)
	{
		pre = cur;
		if(strcmp(new->key, cur->key) < 0)
			cur = cur->l;
		else
			cur = cur->r;
	}
	if(pre == NULL)
		bt_root = new;
	else if(strcmp(new->key, pre->key) < 0)
		pre->l = new;
	else
		pre->r = new;
}
/*
	Print in-order of the tree
*/
void print_bt(bt * cur)
{
	if(cur->l)
		print_bt(cur->l);
	printf("ok %s\n", cur->key);
	if(cur->r)
		print_bt(cur->r);
}
/*
	Delete cur node
*/
void delete_bt(bt * cur)
{
	if(cur->l)
		delete_bt(cur->l);
	if(cur->r)
		delete_bt(cur->r);
	free(cur->key);
	free(cur);
}

/*
	Find all the resources called name and 
	store them in order in the binary tree
*/
void find_recursive(char * name, node * cur)
{
	char * path;	// path that needs to be added
	char * tmp;
	node * prec;	// parent node
	int i;

	for(i = 0;i < MAX_DIRS;i++)
		if(cur->nodes[i] != NULL)
		{
			if(strcmp(name, cur->nodes[i]->name) == 0)
			{
				// I found a valid node, I have to add it to the tree

				path = malloc(strlen(name) + 2);
				path[0] = '\0';
				strcat(path, "/");
				strcat(path, name);

				// To build the string of the entire path 
				// I climb the tree up adding every name
				// to the beginning of the string until
				// I reach the root
				prec = cur;
				while(prec != root)
				{
					tmp = malloc(strlen(path) + strlen(prec->name) + 2);
					tmp[0] = '\0';

					strcat(tmp, "/");
					strcat(tmp, prec->name);
					strcat(tmp, path);
					free(path);
					path = tmp;
					tmp = NULL;

					prec = prec->parent;
				}

				insert_bt(init_bt(path));
			}

			if(cur->nodes[i]->type == DIR)
				find_recursive(name, cur->nodes[i]);
		}
}

/*
	Find and print all the resources with the 
	specified name in lexicografical order
*/
void find(char * parameters)
{
	find_recursive(parameters, root);
	if(bt_root)
	{
		print_bt(bt_root);
		delete_bt(bt_root);
		bt_root = NULL;
	}
	else
		printf("no\n");
}