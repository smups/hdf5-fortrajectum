/*-------------------------------------------------------------------------
 * Copyright (C) 1997	National Center for Supercomputing Applications.
 *			All rights reserved.
 *
 *-------------------------------------------------------------------------
 *
 * Created:		H5ACprivate.h
 *			Jul  9 1997
 *			Robb Matzke <matzke@llnl.gov>
 *
 * Purpose:		Constants and typedefs available to the rest of the
 *			library.
 *
 * Modifications:	
 *
 *-------------------------------------------------------------------------
 */
#ifndef _H5ACprivate_H
#define _H5ACprivate_H
#include <H5ACpublic.h>		/*public prototypes			  */

/* Pivate headers needed by this header */
#include <H5private.h>
#include <H5Fprivate.h>

/*
 * Feature: Define H5AC_DEBUG on the compiler command line if you want to
 *	    debug H5AC_protect() and H5AC_unprotect() by insuring that
 *	    nothing  accesses protected objects.  NDEBUG must not be defined
 *	    in order for this to have any effect.
 */
#ifdef NDEBUG
#  undef H5AC_DEBUG
#endif

/*
 * Class methods pertaining to caching.	 Each type of cached object will
 * have a constant variable with permanent life-span that describes how
 * to cache the object.	 That variable will be of type H5AC_class_t and
 * have the following required fields...
 *
 * LOAD:	Loads an object from disk to memory.  The function
 *		should allocate some data structure and return it.
 *
 * FLUSH:	Writes some data structure back to disk.  It would be
 *		wise for the data structure to include dirty flags to
 *		indicate whether it really needs to be written.	 This
 *		function is also responsible for freeing memory allocated
 *		by the LOAD method if the DEST argument is non-zero.
 */
typedef enum H5AC_subid_t {
    H5AC_BT_ID = 0,		/*B-tree nodes			*/
    H5AC_SNODE_ID = 1,		/*symbol table nodes		*/
    H5AC_HEAP_ID = 2,		/*object or name heap		*/
    H5AC_OHDR_ID = 3,		/*object header			*/
    H5AC_NTYPES = 4		/*THIS MUST BE LAST! */
} H5AC_subid_t;

typedef struct H5AC_class_t {
    H5AC_subid_t	    id;
    void		   *(*load) (H5F_t *, const haddr_t *addr,
				     const void *udata1, void *udata2);
    herr_t		    (*flush) (H5F_t *, hbool_t dest,
				      const haddr_t *addr, void *thing);
} H5AC_class_t;

/*
 * A cache has a certain number of entries.  Objects are mapped into a
 * cache entry by hashing the object's file address.  Each file has its
 * own cache, an array of slots.
 */
#define H5AC_NSLOTS	10330	/*prime number tend to work best  */
#define H5AC_HASH(F,ADDR_P) H5F_addr_hash(ADDR_P,(F)->shared->cache->nslots)

typedef struct H5AC_prot_t {
    const H5AC_class_t	   *type;	/*type of protected thing	*/
    haddr_t		    addr;	/*address of protected thing	*/
    void		   *thing;	/*(possible) protected thing	*/
} H5AC_prot_t;

typedef struct H5AC_slot_t {
    const H5AC_class_t	   *type;	/*type of object stored here	*/
    haddr_t		    addr;	/*file address for object	*/
    void		   *thing;	/*the thing which is cached	*/
#ifdef H5AC_DEBUG
    intn		    nprots;	/*number of things protected	*/
    intn		    aprots;	/*nelmts of `prot' array	*/
    H5AC_prot_t		   *prot;	/*array of protected things	*/
#endif
} H5AC_slot_t;

typedef struct H5AC_t {
    intn		    nslots;	/*number of cache slots		*/
    H5AC_slot_t		   *slot;	/*the cache slots		*/
    intn		    nprots;	/*number of protected objects	*/
    struct {
	uintn			nhits;	/*number of cache hits		*/
	uintn			nmisses;/*number of cache misses	*/
	uintn			ninits;	/*number of cache inits		*/
	uintn			nflushes;/*number of flushes to disk	*/
    } diagnostics[H5AC_NTYPES]; /*diagnostics for each type of object	*/
} H5AC_t;

/*
 * Library prototypes.
 */
herr_t			H5AC_dest(H5F_t *f);
void		       *H5AC_find_f(H5F_t *f, const H5AC_class_t *type,
				    const haddr_t *addr, const void *udata1,
				    void *udata2);
void		       *H5AC_protect(H5F_t *f, const H5AC_class_t *type,
				     const haddr_t *addr, const void *udata1,
				     void *udata2);
herr_t			H5AC_unprotect(H5F_t *f, const H5AC_class_t *type,
				       const haddr_t *addr, void *thing);
herr_t			H5AC_flush(H5F_t *f, const H5AC_class_t *type, const haddr_t *addr,
				   hbool_t destroy);
herr_t			H5AC_create(H5F_t *f, intn size_hint);
herr_t			H5AC_rename(H5F_t *f, const H5AC_class_t *type,
			  const haddr_t *old_addr, const haddr_t *new_addr);
herr_t			H5AC_set(H5F_t *f, const H5AC_class_t *type, const haddr_t *addr,
				 void *thing);
herr_t			H5AC_debug(H5F_t *f);

#define H5AC_find(F,TYPE,ADDR_P,UDATA1,UDATA2)				      \
   (((F)->shared->cache->slot[H5AC_HASH(F,ADDR_P)].type==(TYPE) &&	      \
     H5F_addr_eq (&((F)->shared->cache->slot[H5AC_HASH(F,ADDR_P)].addr),      \
		  ADDR_P)) ?						      \
    ((F)->shared->cache->diagnostics[(TYPE)->id].nhits++,		      \
     (F)->shared->cache->slot[H5AC_HASH(F,ADDR_P)].thing) :		      \
    H5AC_find_f (F, TYPE, ADDR_P, UDATA1, UDATA2))

#endif /* !_H5ACprivate_H */
