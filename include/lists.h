#ifndef _LISTS_H_
#define _LISTS_H_

#define LIST_START    	-1      /* Handy Constants that substitute for item positions */
#define LIST_END      	0       /* END_OF_LIST means one past current length of list when */
				/* inserting. Otherwise it refers the last item in the list. */

typedef struct
    {
    void            *ptr;
    unsigned int    size;
    } HandleRecord;

typedef void **Handle;

typedef int (*CompareFunction)(void *data1, void *data2) ;

typedef struct ListStructTag
    {
    int signature;              /* debugging aid */
    int percentIncrease;        /* %of current size to increase by when list is out of space */
    int minNumItemsIncrease;    /* fixed number of items to increase by when list is out of space */
    int listSize;               /* number of items than can fit in the currently allocated memory */
    int itemSize;               /* the size of each item in the list (same for every item) */
    int numItems;               /* number of items currently in the list */
    unsigned char itemList[1];  /* resizable array of list elements */
    } ListStruct;

typedef struct ListStructTag **list_t;        /* The list abstract data type */
typedef int ( * ListApplicationFunc)(int index, void *ptrToItem, void *callbackData);

/* Basic List Operations */
list_t	ListCreate(int elementSize);
int     ListNumItems(list_t list);
int     ListInsertItem(list_t list, void *ptrToItem, int itemPosition);
int     ListInsertItems(list_t list, void *ptrToItems, int firstItemPosition, int numItemsToInsert);
void    ListDispose(list_t list);
void    *ListGetPtrToItem(list_t list, int itemPosition);
void    ListRemoveItem(list_t list, void *itemDestination, int itemPosition);
void    ListRemoveItems(list_t list, void *itemsDestination, int firstItemPosition, int numItemsToRemove);


#endif	/* _LISTS_H_ */
