/*
 * NanoWM - Window Manager for Nano-X
 *
 * Copyright (C) 2000, 2003, 2010 Greg Haerr <greg@censoft.com>
 * Copyright (C) 2000 Alex Holden <alex@linuxhacker.org>
 */

void wm_init(void)
{
	GR_WM_PROPERTIES props;
	win window;
	static int init = 0;
  
	if(!init) {
		init = 1;

		/* add root window*/
		window.wid = GR_ROOT_WINDOW_ID;
		window.pid = GR_ROOT_WINDOW_ID;
		window.type = WINDOW_TYPE_ROOT;
		window.clientid = 1;
		window.sizing = GR_FALSE;
		window.active = 0;
		window.data = NULL;
		wm_add_window(&window);

		/* when window mgr linked in, this sets update only for current client of root window*/
		GrSelectEvents(GR_ROOT_WINDOW_ID, GR_EVENT_MASK_CHLD_UPDATE);

		/* Set new root window background color*/
		props.flags = GR_WM_FLAGS_BACKGROUND;
		props.background = GrGetSysColor(GR_COLOR_DESKTOP);
		GrSetWMProperties(GR_ROOT_WINDOW_ID, &props);
	}
	else {
		/* must reselect root window child update events for each client when linked in*/
		GrSelectEvents(GR_ROOT_WINDOW_ID, GR_EVENT_MASK_CHLD_UPDATE);
	}
}

/* return 1 if event was handled by window manager and should be changed to no event*/
int wm_handle_event(GR_EVENT *event)
{
	switch(event->type) {
    case GR_EVENT_TYPE_EXPOSURE:
		return wm_exposure(&event->exposure);
    case GR_EVENT_TYPE_BUTTON_DOWN:
		return wm_button_down(&event->button);
    case GR_EVENT_TYPE_BUTTON_UP:
		return wm_button_up(&event->button);
    case GR_EVENT_TYPE_MOUSE_ENTER:
		return wm_mouse_enter(&event->general);
    case GR_EVENT_TYPE_MOUSE_EXIT:
		return wm_mouse_exit(&event->general);
    case GR_EVENT_TYPE_MOUSE_POSITION:
		return wm_mouse_moved(&event->mouse);
    case GR_EVENT_TYPE_KEY_DOWN:
		return wm_key_down(&event->keystroke);
    case GR_EVENT_TYPE_KEY_UP:
		return wm_key_up(&event->keystroke);
    case GR_EVENT_TYPE_FOCUS_IN:
		return wm_focus_in(&event->general);
    case GR_EVENT_TYPE_CHLD_UPDATE:	/* the frame's child had an update, update frame*/
		return wm_update(&event->update);
	case GR_EVENT_TYPE_NONE:
	case GR_EVENT_TYPE_TIMER:
	case GR_EVENT_TYPE_UPDATE:		/* no need for frame event handling*/
		break;
	case GR_EVENT_TYPE_ERROR:
		break;
    default:
		break;
    }
    return 0;
}

int wm_exposure(GR_EVENT_EXPOSURE *event)
{
	win *window;


	if(!(window = wm_find_window(event->wid)))
		return 0;

	switch(window->type) {
		case WINDOW_TYPE_CONTAINER:
			wm_container_exposure(window, event);
			return 0; 	/* don't eat event*/
		default:
			break;
	}
	return 0;
}

int wm_button_down(GR_EVENT_BUTTON *event)
{
	win *window;


	if(!(window = wm_find_window(event->wid)))
		return 0;

	switch(window->type) {
		case WINDOW_TYPE_CONTAINER:
			wm_container_buttondown(window, event);
			return 1; 	/* eat event*/
		default:
			break;
	}
	return 0;
}

int wm_button_up(GR_EVENT_BUTTON *event)
{
	win *window;


	if(!(window = wm_find_window(event->wid)))
		return 0;

	switch(window->type) {
		case WINDOW_TYPE_CONTAINER:
			wm_container_buttonup(window, event);
			return 1;	/* eat event*/
		default:
			break;
	}
	return 0;
}

int wm_mouse_enter(GR_EVENT_GENERAL *event)
{
	win *window;


	if(!(window = wm_find_window(event->wid)))
		return 0;

	switch(window->type) {
		case WINDOW_TYPE_CONTAINER:
			wm_container_mouse_enter(window, event);
			return 0; 	/* don't eat event*/
		default:
			break;
	}
	return 0;
}

int wm_mouse_exit(GR_EVENT_GENERAL *event)
{
	win *window;


	if(!(window = wm_find_window(event->wid)))
		return 0;

	switch(window->type) {
		case WINDOW_TYPE_CONTAINER:
			wm_container_mouse_exit(window, event);
			return 0; 	/* don't eat event*/
		default:
			break;
	}
	return 0;
}

int wm_mouse_moved(GR_EVENT_MOUSE *event)
{
	win *window;


	if(!(window = wm_find_window(event->wid)))
		return 0;

	switch(window->type) {
		case WINDOW_TYPE_CONTAINER:
			wm_container_mousemoved(window, event);
			return 0; 	/* don't eat event*/
		default:
			break;
	}
	return 0;
}

int wm_focus_in(GR_EVENT_GENERAL *event)
{
	win *window;


	if(!(window = wm_find_window(event->wid)))
		return 0;

	switch(window->type) {
		default:
			break;
	}
	return 0;
}

int wm_key_down(GR_EVENT_KEYSTROKE *event)
{

	/* FIXME: Implement keyboard shortcuts */
	return 0;
}

int wm_key_up(GR_EVENT_KEYSTROKE *event)
{
	return 0;
}

int wm_update(GR_EVENT_UPDATE *event)
{
	win *window;

	
	if(!(window = wm_find_window(event->subwid))) {
		if (event->utype == GR_UPDATE_MAP)
			wm_new_client_window(event->subwid);
	  	return 0;
	}

	if(window->type == WINDOW_TYPE_CONTAINER) {
		if (event->utype == GR_UPDATE_ACTIVATE)
			wm_redraw_ncarea(window);
		return 0;
	}

	if (window->type == WINDOW_TYPE_CLIENT) {
		if(event->utype == GR_UPDATE_MAP)
			wm_client_window_remap(window);
		if(event->utype == GR_UPDATE_DESTROY)
			wm_client_window_destroy(window);
		if(event->utype == GR_UPDATE_UNMAP)
			wm_client_window_unmap(window);
		if(event->utype == GR_UPDATE_SIZE)
			wm_client_window_resize(window);
	}
	return 0;
}
