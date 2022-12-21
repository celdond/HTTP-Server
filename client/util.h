#ifndef __UTIL_H__
#define __UTIL_H__

struct link_list {
	int length;
	struct node *head;
};

struct node {
	char command;
	char *file_name;
	struct node *next;
};

struct link_list *create_list();

struct node *insert_node(struct node *n);

void delete_node(struct node *n);

void delete_list(struct link_list *l);

#endif