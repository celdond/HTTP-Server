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
