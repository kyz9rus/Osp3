#include <inttypes.h>
#include <stdbool.h>
#include "TMessage.h"

typedef struct _pointer_t {
    int count;
    struct _node_t *ptr;
} pointer_t;

typedef struct _node_t {
    pointer_t next;
    TMessage *tMessage;
} node_t;

typedef struct ConcurrentQueue {
    pointer_t head;
    pointer_t tail;
} ConcurrentQueue;

struct ConcurrentQueue *init_queue(void);

void free_queue(ConcurrentQueue *);

bool enqueue(ConcurrentQueue *q, TMessage *tMessage);

TMessage *dequeue(ConcurrentQueue *q);

void show_queue(ConcurrentQueue *);