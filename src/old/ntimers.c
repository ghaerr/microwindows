
void
GsAddTimer(struct GsTimer* newtimer) {
    int i;
    struct GsTimer* curTimer;
    if(our_timers.numof++>0) {
	curTimer=our_timer->head;
	for(curTimer=our_timer->head;curTimer!=NULL;curTimer=curTimer->next) {
	    if(curTimer->due>newtimer->due) { /* time to file it away... */
		if(our_timer->head==curTimer) { /* put it at the beginning... */
		    newtimer->next=our_timer->head;
		    our_timer->head=newtimer;
		    return;
		} /* else put it in the middle... */
		newtimer->next=curTimer;
		prevTimer->next=newtimer;
		return;
	    }
	    prevTimer=curTimer;
	} /* else put it at the end... */
	our_timer->tail->next=newtimer;
	our_timer->tail=newtimer;
	newtimer->next=NULL;
	return;
    } /* else... hey it's the first timer! */
    our_timer->head=newtimer;
    our_timer->tail=newtimer;
    newtimer->next=NULL;    
}
