/*
 * NanoClasses v0.1
 * (C) 1999 by Screen Media
 * 
 * Minimal toolkit to build a C based class hierarchy
 * 
 */

#ifndef __NCLASS_H
#define __NCLASS_H

#define NWTRUE 1
#define NWFALSE 0

#define NCLASS(__c__) (__ ## __c__ ## _nclass)
#define DEFINE_NCLASS(__c__,__super__) \
struct __c__ ## _nclass; struct __c__ ## _nclass __ ## __c__ ## _nclass; \
struct __c__ ## _nclass { \
    struct __super__ ## _nclass __data;

#define END_NCLASS };

#define DEFINE_NOBJECT(__class__,__super__) \
  struct __class__ ## _nobject { struct __super__ ## _nobject __super;
#define END_NOBJECT };

#define INIT_NCLASS(__class__,__super__) \
static int __ ## __class__ ## _class_init = 0;\
void n_init_ ## __class__ ## _class (void) {\
   struct __class__ ## _nclass * this = &__ ## __class__ ## _nclass; \
   if (__ ## __class__ ## _class_init) return; \
   n_init_ ## __super__ ## _class (); \
   memcpy(&this->__data,&__ ## __super__ ## _nclass,sizeof(struct __super__ ## _nclass)); \
   ((struct nclass *)this)->__super = (struct nclass *)&__ ## __super__ ## _nclass; \
   __ ## __class__ ## _class_init = 1;
#define END_INIT  }

#define NMETHOD(__class__,__slot__,__func__) \
  ((struct __class__ ## _nclass *)this)->##__slot__##_func = ##__func__;

#define NSLOT(__ret__,__name__) __ret__ (* __name__ ## _func) ()

#ifdef DEBUG
# define n_call(__class__,__slot__,__object__,__args__) \
  (fprintf(stderr,__FILE__ ",line %d: %p::" # __class__ "_" # __slot__ # __args__ "\n",__LINE__,__object__)), \
  ((struct __class__ ## _nclass *)(((NOBJECT *)__object__)->__class))->##__slot__##_func ## __args__
#else
# define n_call(__class__,__slot__,__object__,__args__) \
  ((struct __class__ ## _nclass *)(((NOBJECT *)__object__)->__class))->##__slot__##_func ## __args__
#endif

#define n_super(__class__,__slot__,__object__, __args__) \
  ((struct __class__ ## _nclass *)((struct nclass *)(((NOBJECT *)__object__)->__class)->__super))->##__slot__##_func ## __args__

#define NEW_NOBJECT(__class__) ((struct __class__ ## _nobject *)n_new_object((NCLASS *)&__ ## __class__ ## _nclass, sizeof(struct __class__ ## _nobject)))
#define DELETE_OBJECT(__ob__) n_delete_object((NOBJECT *)__ob__)

struct nclass {
   struct nclass * __super;
};

struct nobject {
   struct nclass * __class;
};

typedef struct nobject NOBJECT;
typedef struct nclass NCLASS;

struct object_nobject {
   NOBJECT __super;
};

struct object_nclass {
   NCLASS * __super;
   
   NSLOT(int,init);
   NSLOT(void,cleanup);
};

void n_init_object_class(void);
extern struct object_nclass __object_nclass;

#define n_object_init(__this__) n_call(object,init,__this__,(__this__))
#define n_object_cleanup(__this__) n_call(object,cleanup,__this__,(__this__))

NOBJECT * n_new_object(NCLASS * c, int size);
void n_delete_object(NOBJECT * ob);

#endif
