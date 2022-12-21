#include "util.h"

struct link_list *create_list() {
	struct link_list *l = (struct link_list *)calloc(1, sizeof(struct link_list));
	if (!l) {
		return ((struct link_list *) NULL);
	}
	l->length = 0;
	l->head = NULL;

	return l;
}

struct node *insert_node(struct node *n) {
	struct node *new = (struct node *)calloc(1, sizeof(struct node));
        if (!new) {
                return ((struct node *) NULL);
        }
	new->command = '0';
	new->file_name = NULL;
	new->next = NULL;

	n->next = new;
	return new;
}
