/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * File: linkedlist.h
 * Purpose: C implementation of LinkedList data structure.
 * The following two macros are define here:
 * 		LN_VSTATIC : Values in ListNode do not need to be free'd
 *  	LN_VDYNAMIC: Values in ListNode need to be free'd
 * These macros are passed when deleting LinkedLists
 * @author: Bryan Rocha
 * @version: 1.0 (2/3/2019)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef LINKEDLIST_H
#define LINKEDLIST_H

#define LN_VSTATIC  0
#define LN_VDYNAMIC 1

struct list_node
{
	struct list_node *next;
	void *value;
};

struct linked_list
{
	struct list_node *front;
	struct list_node *rear;
};

struct linked_list *create_list();

void insert_front(struct linked_list *, void *);
void insert_rear(struct linked_list *, void *);
void remove_from_list(struct linked_list *, void *);

void delete_linked_list(struct linked_list **, short);
void delete_list(struct list_node *, short);

#endif
