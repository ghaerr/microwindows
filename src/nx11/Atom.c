#include <string.h>
#include <stdlib.h>
#include "X11/Xatom.h"
#include "nxlib.h"

#define SZHASHTABLE	64

struct hash_t {
	char *name;
	Atom atom;
	struct hash_t *next;
};

static struct hash_t *hash_list[SZHASHTABLE];
static unsigned long atom_id = XA_LAST_PREDEFINED + 32;

static unsigned char
hash_str(_Xconst char *name)
{
	unsigned char ch = 0;
	int i = 0;

	for (i = 0; i < strlen(name); i++)
		ch += name[i];

	return (ch % SZHASHTABLE);
}

Atom
XInternAtom(Display * display, _Xconst char *atom_name, Bool only_if_exists)
{
	unsigned char hash = hash_str(atom_name);
	struct hash_t *val;

	for (val = hash_list[hash]; val; val = val->next)
		if (strcmp(val->name, atom_name) == 0) {
			return val->atom;
		}

	if (only_if_exists == True)
		return None;

	if (!hash_list[hash])
		val = hash_list[hash] =
			(struct hash_t *) Xcalloc(1, sizeof(struct hash_t));
	else {
		struct hash_t *h = hash_list[hash];
		while (h->next)
			h = h->next;
		val = h->next =
			(struct hash_t *) Xcalloc(1, sizeof(struct hash_t));
	}

	val->name = strdup(atom_name);
	val->atom = atom_id++;
	return val->atom;
}

Status
XInternAtoms(Display * display, char **names, int count,
	     Bool only_if_exists, Atom * atoms_return)
{
	int i;
	int ret = 1;

	for (i = 0; i < count; i++) {
		atoms_return[i] =
			XInternAtom(display, names[i], only_if_exists);
		if (!atoms_return[i])
			ret = 0;
	}

	return ret;
}

/* return copy of Atom name*/
char *
XGetAtomName(Display * display, Atom atom)
{
	int i;

	for (i = 0; i < SZHASHTABLE; i++) {
		struct hash_t *val;

		for (val = hash_list[i]; val; val = val->next)
			if (val->atom == atom) {
				char *name = strdup(val->name);
				return name;
			}
	}
	return NULL;
}

Status
XGetAtomNames(Display * display, Atom * atoms, int count, char **names_return)
{
	int i;
	int ret = 1;

	for (i = 0; i < count; i++) {
		names_return[i] = XGetAtomName(display, atoms[i]);
		if (!names_return[i])
			ret = 0;
	}

	return ret;
}
