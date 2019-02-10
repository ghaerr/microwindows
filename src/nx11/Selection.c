/* Copyright 2003, Jordan Crouse (jordan@cosmicpenguin.net) */

#include "nxlib.h"
#include <stdlib.h>
#include <time.h>

/* Yet another dummy storage area.  We hold onto the value and
   return them when XGetSelectionOwner says so

   It seems that selections live forever - I don't see any way to
   remove them.  That's odd to me, but oh, well......
*/

struct sstruct {
	Atom selection;
	Window owner;
	Time time;
	struct sstruct *next;
};

static struct sstruct *select_list = 0;

int
XSetSelectionOwner(Display *display, Atom selection, Window owner, Time t)
{
	struct sstruct *select = select_list;
	struct sstruct *prev = 0;
	
	for( ; select; prev = select, select = select->next) {
		if (select->selection == selection) {
			if (t > select->time) { 
				/* Note:  SelectionClear event should get generated here */
				select->owner = owner;
				select->time = (t == CurrentTime) ? time(0) : t;
			}
			return 0;
		}
	}
	select = (struct sstruct *)Xmalloc(sizeof(struct sstruct));
	select->selection = selection;
	select->owner = owner;
	select->time = (t == CurrentTime)? time(0) : t;
	select->next = 0;

	if (!prev)
		select_list = select;
	else prev->next = select;

	return 0;
}

static struct sstruct *
find_selection(Atom selection)
{
	struct sstruct *select = select_list;
	
	for (; select; select = select->next) 
		if (select->selection == selection)
			return select;
	return 0;
}

Window
XGetSelectionOwner(Display *display, Atom selection)
{
	struct sstruct *select = find_selection(selection);

	return (select)? select->owner : None;
}

/* 
 * Move selection to target.  There are lots of events that might be
 * generated here, but for now we won't worry about those. 
 */
int
XConvertSelection(Display *display, Atom selection, Atom target, Atom property,
	Window requestor, Time t)
{
	struct sstruct *select = find_selection(selection);
	
	if (select) {
		select->selection = target;
		select->time = (t == CurrentTime) ? time(0) : t;
	}

	/* We should generate a SelectionRequest event to the owner in this 
	   situation, or if there is no owner, then a SelectionNotify 
	   to the requestor 
	*/
	return 0;
}
