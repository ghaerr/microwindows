/*
 * Nano-X server routines for not UNIX && HAVE_SELECT
 */

/* MSDOS | _MINIX | NDS | VXWORKS | PSP | __MINGW32__ | ALLEGRO | EMSCRIPTEN*/

/* this GsSelect() is used for all polling-based platforms not specially handled above*/
#define WAITTIME	50		/* blocking sleep interval in msecs unless polling*/

void 
GsSelect(GR_TIMEOUT timeout)
{
	int numevents = 0;
	GR_TIMEOUT waittime = 0;
	GR_EVENT_GENERAL *gp;

	/* input gathering loop */
	while (1)
	{
		/* perform single update of aggregate screen update region*/
		if(scrdev.PreSelect)
			scrdev.PreSelect(&scrdev);

		/* poll for mouse data and service if found*/
		while (GsCheckMouseEvent())
			if (++numevents > 10)
				break;				/* don't handle too many events at one shot*/

		/* poll for keyboard data and service if found*/
		while (GsCheckKeyboardEvent())
			if (++numevents > 10)
				break;				/* don't handle too many events at one shot*/
		
		/* did we handle any input or were we just polling?*/
		if (numevents || timeout == (GR_TIMEOUT)-1L)
			return;					/* yes - return without sleeping*/

		/* give up time-slice & sleep for a bit */
		//GdDelay(WAITTIME);
		waittime += WAITTIME; 

		/* have we timed out? */
		if (waittime >= timeout)
		{
			/* special case: polling when timeout == 0 -- don't send timeout event */
			if (timeout != 0)
			{
				/* Timeout has occured.  
				 * Currently return a timeout event regardless of whether client 
				 * has selected for it.
				 */
				if ((gp = (GR_EVENT_GENERAL *)GsAllocEvent(curclient)) != NULL)
					gp->type = GR_EVENT_TYPE_TIMEOUT;
			}
			return;
		}
	}
}
