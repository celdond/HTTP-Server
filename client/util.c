#include "util.h"

// create_list:
// create a linked list structure
// return: head of the linked list
struct link_list *create_list() {
  struct link_list *l = (struct link_list *)calloc(1, sizeof(struct link_list));
  if (!l) {
    return ((struct link_list *)NULL);
  }
  l->length = 0;
  l->head = NULL;
  l->tail = NULL;

  return l;
}

// insert_node:
// create a new node in the linked list
// l: linked list to add a node to
// return: reference to the new node
struct node *insert_node(struct link_list *l) {
  struct node *new = (struct node *)calloc(1, sizeof(struct node));
  if (!new) {
    return ((struct node *)NULL);
  }
  new->command = '0';
  new->file_name = NULL;
  new->next = NULL;

  if (l->tail) {
    l->tail->next = new;
  } else {
    l->head = new;
  }
  l->tail = new;
  l->length += 1;

  return new;
}

// delete_node:
// deletes the input node
// n: node to delete
void delete_node(struct node *n) {
  if (n->file_name) {
    free(n->file_name);
  }
  free(n);
  n = NULL;
  return;
}

// delete_list:
// frees all the allocated memory to the linked list
// link_list: list to delete
void delete_list(struct link_list *l) {
  int node_count = l->length;
  l->length = 0;
  l->tail = NULL;
  struct node *n = l->head;
  if (!n) {
    free(l);
    return;
  }

  struct node *next = n->next;
  for (int i = 0; i < node_count; i++) {
    delete_node(n);
    n = next;
    if (n) {
      next = n->next;
    }
  }
  l->head = NULL;
  free(l);
  return;
}
