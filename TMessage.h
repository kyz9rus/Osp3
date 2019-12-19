#include <stdint.h>

typedef enum EType {
    FIBONACCI,
    POW,
    BUBBLE_SORT_UINT64,
    STOP
} EType;

typedef struct {
    uint8_t Type;
    uint64_t Size;
    long *Data;
} TMessage;