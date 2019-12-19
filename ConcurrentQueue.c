#include "ConcurrentQueue.h"

#include <stdio.h>
#include <stdlib.h>

cas(volatile pointer_t *addr, pointer_t oldp, const pointer_t newp) {
    char result;
    __asm__ __volatile__("lock; cmpxchg16b %0; setz %1":"=m"(*addr),
    "=q"(result)
    :"m"(*addr), "a"(oldp.count), "d"(oldp.ptr),
    "b"(newp.count), "c"(newp.ptr)
    :"memory");
    return (((int) result == 0) ? false : true);
}

static node_t *create_node(const TMessage *tMessage) {
    node_t *node;
//     if ((node = (node_t *) calloc(1, sizeof(node_t))) == NULL) {
    if ((node = (node_t *) calloc(1, sizeof(node_t))) == NULL) {
        return NULL;
    }

    node->tMessage = tMessage;
    node->next.ptr = NULL;
    node->next.count = 0;

    return node;
}

ConcurrentQueue *init_queue(void) {
    ConcurrentQueue *q;
    node_t *node;

    if ((q = (ConcurrentQueue *) calloc(1, sizeof(ConcurrentQueue))) == NULL) {
        return NULL;
    }

    if ((node = create_node((TMessage*) NULL)) == NULL) {
        abort();
    }

    q->head.ptr = node;
    q->tail.ptr = node;

    return q;
}

void free_queue(ConcurrentQueue *q) {
    free(q);
}

bool enqueue(ConcurrentQueue *q, TMessage *tMessage){
    node_t *newNode;
    pointer_t tail, next, tmp;

    if ((newNode = create_node(tMessage)) == NULL)
        return false;

    while (1) {
        tail = q->tail;
        next = tail.ptr->next;

        if (tail.count == q->tail.count && tail.ptr == q->tail.ptr) {
            if (next.ptr == NULL) {
                tmp.ptr = newNode;
                tmp.count = next.count + 1;
                if (cas(&tail.ptr->next, next, tmp) == true) {
                    break;
                }
            } else {
                tmp.ptr = next.ptr;
                tmp.count = tail.count + 1;
                cas(&q->tail, tail, tmp);
            }
        }
    }
    tmp.ptr = newNode;
    tmp.count = tail.count + 1;
    cas(&q->tail, tail, tmp);

    return true;
}

TMessage *dequeue(ConcurrentQueue *q) {
    pointer_t head, tail, next, tmp;

    while (1) {
        head = q->head;
        tail = q->tail;
        next = head.ptr->next;

        if (head.count == q->head.count && head.ptr == q->head.ptr) {
            if (head.ptr == tail.ptr) {
                if (next.ptr == NULL) {
                    return (TMessage*) NULL;
                }
                tmp.ptr = next.ptr;
                tmp.count = head.count + 1;
                cas(&q->tail, tail, tmp);
            } else {
//                *val = next.ptr->tMessage;
                tmp.ptr = next.ptr;
                tmp.count = head.count + 1;
                if (cas(&q->head, head, tmp) == true) {
                    return tmp.ptr->tMessage; // TODO: see and fix
                }
            }
        }
    }

    free(head.ptr);
    return true;
}

void show_queue(ConcurrentQueue *q) {
    node_t *curr;

    curr = q->head.ptr;
    while ((curr = curr->next.ptr) != NULL) {
        printf("[%d]", (int) curr->tMessage);
    }
    printf("\n");

}