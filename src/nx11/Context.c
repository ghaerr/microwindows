//---------------------------------------------------------
//	2009 Yuichiro Nakada
//---------------------------------------------------------
#include <stdlib.h>

#if 0
#include <stdio.h>

void* xalloc(size_t sz)
{
	void* p;
	p = calloc(1, sz);
	if (!p) exit(1);
	return p;
}
/* Iterator */
typedef struct _iterator {
	void* element;
	struct _iterator* next;
} Iterator;
/* Linked List */
typedef struct _list {
	int element_size;
	struct _iterator* head;
	struct _iterator* tail;
} List;

/* Linked List Generation */
#define list_alloc(list, type) \
	list = xalloc(sizeof(List)); \
	list->element_size = sizeof(type);

/* Iterator Of the element to be stored in alloc */
void* list_new_element(List* list)
{
	return xalloc(list->element_size);
}
/* IteratorAcquisition */
Iterator* get_iterator(List* list)
{
	return list->head;
}
/* Do you have elements? */
int iterator_has_value(Iterator* ite)
{
	return ite != NULL;
}
/* Get the next element */
void* iterator_next(Iterator* ite)
{
	return ite->next;
}
/* Linked List Add an object to */
void list_add(List* list, void* new_element)
{
	if (list->tail == NULL) {
		/* First element */
		list->tail = list->head = xalloc(sizeof(Iterator));
	} else {
		Iterator* old_tail = list->tail;
		old_tail->next = xalloc(sizeof(Iterator));
		list->tail = old_tail->next;
	}
	list->tail->element = new_element;
}
/* Linked List Open */
void list_free(List* list)
{
	Iterator* it = get_iterator(list);
	while (1) {
		Iterator* old_it = it;
		if (it == NULL) break;
		free(it->element);
		it = it->next;
		free(old_it);
	}
}
/* How old are you? foreach */
#define foreach(it, list) for(it = get_iterator(list); iterator_has_value(it); it = iterator_next(it))


typedef struct _point {
    int x;
    int y;
} Point;                        /*Suitable structure samples */
int main(int argc, char** argv)
{
    List* list;
    Iterator* it;
    int i;
    list_alloc(list, Point);    /* Make a list */
    for (i = 0; i < 10; i++) {
        Point* p1 = list_new_element(list);  /* Store it in the list element Give an area of */
        p1->x = i;
        p1->y = i;
        list_add(list, p1);     /* Point Add a structure to the list */
    }
    foreach (it, list) {        /* How to use foreach */
        Point* p = it->element;
        DPRINTF("point: (%d, %d)\n", p->x, p->y);
    }
    list_free(list);
    return 0;
}
#endif


#include "nxlib.h"
#include "X11/Xutil.h"

typedef struct _XCList {
	Display *display;
	XID rid;
	XContext context;
	XPointer data;

	struct _XCList	*prev;
	struct _XCList	*next;
} XCList;

XCList xcl;

int XFindContext(Display *display, XID rid, XContext context, XPointer *data)
{
	DPRINTF("XFindContext called...\n");
	XCList *p = &xcl;
	while (p->next) {
		if (p->display == display && p->rid == rid && p->context == context) {
			*data = p->data;
			return 0;
		}
		p=p->next;
	}
	return XCNOENT;
}

#if NeedFunctionPrototypes
int XSaveContext(Display *display, register XID rid, register XContext context, _Xconst char* data)
#else
int XSaveContext(Display *display, XID rid, XContext context, XPointer data)
#endif
{
	DPRINTF("XSaveContext called...\n");
	XCList *p = &xcl;
	while (p->next) {
		if (p->display == display && p->rid == rid && p->context == context) {
			p->data = (char*)data;	// over write
			return 0;
		}
		p=p->next;
	}

	p->next = calloc(1, sizeof(XCList));	// with 0 clear
	if (!p->next) return XCNOMEM;
	p->next->prev=p;
	p=p->next;

	p->display = display;
	p->rid = rid;
	p->context = context;
	p->data = (char*)data;
	return 0;
}
//#endif
int XDeleteContext(Display *display, XID rid, XContext context)
{
	DPRINTF("XDeleteContext called...\n");
	XCList *p = &xcl;
	while (p->next) {
		if (p->display == display && p->rid == rid && p->context == context) {
			p->prev->next = p->next;
			p->next->prev = p->prev;
			free(p);
			return 0;
		}
		p=p->next;
	}
	return XCNOENT;
}
