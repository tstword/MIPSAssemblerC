//----------------------------------------------------------------
// File: linkedlist.c
// Purpose: C implementation of LinkedList data structure.
// @author: Bryan Rocha
// @version: 1.0 (2/3/2019)
//----------------------------------------------------------------

#include "linkedlist.h"

#include <stdlib.h>

// -------------------------------------------------------------------------
// Dynamically allocates a LinkedList and returns the address
// @return Address of LinkedList structure if successful, otherwise NULL.
// -------------------------------------------------------------------------
struct linked_list *create_list()
{
	struct linked_list *list = (struct linked_list *)malloc(sizeof(struct linked_list));
	list->front = list->rear = NULL;
	return list;
}

// -------------------------------------------------------
// Appends new ListNode to the front of the LinkedList.
// @param list: Pointer to LinkedList structure.
//		  value: The data to insert into the LinkedList
// @additional comments: Executes in O(1) time
// -------------------------------------------------------
void insert_front(struct linked_list *list, void *value)
{
	struct list_node *node = (struct list_node *)malloc(sizeof(struct list_node));
	node->next = list->front;
	node->value = value;

	list->front = node;
	
	if(list->rear == NULL)
	{
		list->rear = node;
	}
}

// -------------------------------------------------------
// Appends new ListNode to the end of the LinkedList.
// @param list: Pointer to LinkedList structure.
//		  value: The data to insert into the LinkedList
// @additional comments: Executes in O(1) time
// -------------------------------------------------------
void insert_rear(struct linked_list *list, void *value)
{
	struct list_node *node = (struct list_node *)malloc(sizeof(struct list_node));
	node->value = value;
	node->next = NULL;

	if(list->rear != NULL)
	{
		list->rear->next = node;
		list->rear = node;
	}
	else
	{
		list->rear = list->front = node;
	}
}

// ---------------------------------------------------
// Removes the ListNode containing the unique value
// @param list: Pointer to LinkedList structure.
//		  value: Unique value to be removed
// @additional comments: Executes in O(n) time
// ---------------------------------------------------
void remove_from_list(struct linked_list *list, void *value)
{
    struct list_node *curr = list->front;
    struct list_node *prev = NULL;

    while(curr != NULL)
    {
        if(curr->value == value)
        {
            if(prev != NULL)
                prev->next = curr->next;

            if(curr == list->front) list->front = curr->next;
            if(curr == list->rear)  list->rear = prev;
			
			free(curr);

            break;
        }

        prev = curr;
        curr = curr->next;
    }
}

// ---------------------------------------------------------------------------
// Frees the LinkedList structure along with the ListNode structures created.
// @param lp: Pointer to the address of the LinkedList structure
// 		  mode: LN_VDYNAMIC -> Frees the values inside the ListNodes
//				LN_VSTATIC  -> Does not free the values inside the ListNodes
// @additional comments: Executes in O(n) time
// ---------------------------------------------------------------------------
void delete_linked_list(struct linked_list **lp, short mode)
{
	if(*lp == NULL) return;
    
	delete_list((*lp)->front, mode);
	free(*lp);
	
	*lp = NULL;
}

// ----------------------------------------------------------------------------
// The ListNodes in an iterative fashion.
// @param listNode: Address of the ListNode structure
// 		  mode: LN_VDYNAMIC -> Frees the values inside the ListNodes
//				LN_VSTATIC  -> Does not free the values inside the ListNodes
// @additional comments: Executes in O(n) time
// ----------------------------------------------------------------------------
void delete_list(struct list_node *listnode, short mode)
{
	struct list_node *node = listnode;
	struct list_node *temp;
	
	while(node != NULL)
	{
		if(mode == LN_VDYNAMIC)
			free(node->value);
	
		temp = node;
		node = node->next;
		free(temp);
	}
}
