#include <stdio.h>
#include <stdlib.h>
#include "device.h"
/*
 * linked list routines
 *
 * 1/28/98 g haerr
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 */

void *
GdItemAlloc(unsigned int size)
{
	return (void *)calloc(size, 1);
}

/* insert at tail of list*/
void
GdListAdd(PMWLISTHEAD pHead,PMWLIST pItem)
{
	if( pHead->tail) {
		pItem->prev = pHead->tail;
		pHead->tail->next = pItem;
	} else
		pItem->prev = NULL;
	pItem->next = NULL;
	pHead->tail = pItem;
	if( !pHead->head)
		pHead->head = pItem;
}

/* insert at head of list*/
void
GdListInsert(PMWLISTHEAD pHead,PMWLIST pItem)
{
	if( pHead->head) {
		pItem->next = pHead->head;
		pHead->head->prev = pItem;
	} else
		pItem->next = NULL;
	pItem->prev = NULL;
	pHead->head = pItem;
	if( !pHead->head)
		pHead->head = pItem;
}

void
GdListRemove(PMWLISTHEAD pHead,PMWLIST pItem)
{
	if( pItem->next)
		pItem->next->prev = pItem->prev;
	if( pItem->prev)
		pItem->prev->next = pItem->next;
	if( pHead->head == pItem)
		pHead->head = pItem->next;
	if( pHead->tail == pItem)
		pHead->tail = pItem->prev;
	pItem->next = pItem->prev = NULL;
}
