#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <zconf.h>
#include <assert.h>
#include "ConcurrentQueue.h"

#define INIT_BUFFER_SIZE 1000
#define MAX_COMMAND_LENGTH 18
#define OUTPUT_FILENAME "result.txt"

void *reader();

void *writeCustom();

void *calc(void *data);

// ------------ THREAD FUNCTIONS ------------
long *calcFib(long *);

long *calcPow(long *);

void *doSort(long *);

// ----------------- UTIL_FUNCTIONS -----------------
int numlen(unsigned int number) {
    int length = 0;

    do {
        length++;
        number = number / 10;
    } while (number + 1 > 1);

    return length;
}

int indexOf(char *string, char symbol) {
    char *findedSymbol = (char *) strchr(string, (int) symbol);
    if (findedSymbol == NULL)
        return -1;

    return (int) (findedSymbol - string);
}

int size(const long *arr) {
    int i = 0;

    while (arr[i] != 0) {
        i++;
    }

    return i;
}

char *readString(size_t size) {
    char *buffer = calloc(INIT_BUFFER_SIZE, size); //TODO: how to free?
    fgets(buffer, 200, stdin);
    buffer[strlen(buffer) - 1] = '\0';

    return buffer;
}

EType stringToEType(char *string) {
    if (strcmp(string, "FIBONACCI") == 0)
        return FIBONACCI;
    else if (strcmp(string, "POW") == 0)
        return POW;
    else if (strcmp(string, "BUBBLE_SORT_UINT64") == 0)
        return BUBBLE_SORT_UINT64;
    else if (strcmp(string, "STOP") == 0)
        return STOP;
}

EType createTMessageType(char *message) {
    char *typeString = calloc(MAX_COMMAND_LENGTH, sizeof(char));
    int indexOfSpace = indexOf(message, ' ');

    if (indexOfSpace != -1) {
        strncpy(typeString, message, indexOfSpace);
        EType eType = stringToEType(typeString);
        free(typeString);
        return eType;
    } else {
        return stringToEType(message);
    }
}

long *convertParameters(char *string) {
    long *result = calloc(100, sizeof(long));
    int index = 0, counter = 0;

    char *argString;
    long arg;

    do {
        char delim[] = " ";

        if (counter == 0) {
            argString = strtok(string, delim);
            counter++;
        } else
            argString = strtok(NULL, delim);

        if (argString == NULL)
            break;

        arg = (long) atoi(argString);
        if (arg == '\0') {
            errno = EINVAL;
            printf("Incorrect parameter: %s\n", argString);
            exit(errno);
        }

        result[index] = arg;
        index++;
    } while (1);

    return result;
}

void fillData(TMessage *tMessage, char *command) {
    int indexOfSpace = indexOf(command, ' ');
    char *buffer = calloc(INIT_BUFFER_SIZE, sizeof(char));
    strncpy(buffer, command + indexOfSpace + 1, strlen(command));

    tMessage->Data = convertParameters(buffer);

    free(buffer);
}

TMessage *createTMessage() {
    TMessage *result = calloc(1, sizeof(TMessage));

    char *buffer = readString(sizeof(uint64_t));
    result->Type = createTMessageType(buffer);
    if (result->Type == STOP) {
        goto exit;
    }

    result->Size = strlen(buffer) * sizeof(long);
    result->Data = calloc(result->Size, sizeof(long));

    fillData(result, buffer);

    free(buffer);

    exit:
    return result;
}

long recursiveFib(long n) {
    if (n == 0 || n == 1)
        return 1;
    else
        return (recursiveFib(n - 1) + recursiveFib(n - 2));
}

void swap(long *val1, long *val2) {
    long temp = *val1;
    *val1 = *val2;
    *val2 = temp;
}

void writeMas(FILE *file, TMessage *tMessage) {
    for (int i = 0; i < size(tMessage->Data); i++)
        fprintf(file, "%hhu ", tMessage->Data[i]);
}

// ------------ END_UTIL_FUNCTIONS ---------------------

pthread_t readerTid, writerTid, calcTid;
pthread_attr_t attr;
ConcurrentQueue *queueForWriting;


void *readTest(void *data) {
    for (int i = 0; i < 100; i++)
        show_queue(queueForWriting);
}

void *writeTest(void *data) {
    for (int i = 0; i < 100; i++)
        enqueue(queueForWriting, i);
}

pthread_t readerTestTid, writerTestTid, metricTid;

void tests() {
    {
        queueForWriting = init_queue();

        pthread_attr_init(&attr);
        pthread_create(&readerTestTid, &attr, readTest, NULL);
        pthread_create(&writerTestTid, &attr, writeTest, NULL);

        pthread_join(readerTestTid, NULL);
        pthread_join(writerTestTid, NULL);
    }

    {
        long *data = calloc(7, sizeof(long));
        data[0] = 4;
        data[1] = 2;
        data[2] = 1;
        data[3] = 3;
        data[4] = 8;
        data[5] = 5;

        doSort(data);

        assert(data[0] == 1);
        assert(data[1] == 2);
        assert(data[2] == 3);
        assert(data[3] == 4);
        assert(data[4] == 5);
        assert(data[5] == 8);
    }

    {
        long *data = calloc(1, sizeof(long)), *result;

        data[0] = 5;
        result = calcFib(data);
        assert(result[0] == (long) 8);

        data[0] = 9;
        result = calcFib(data);
        assert(result[0] == (long) 55);
    }

    {
        long *data = calloc(2, sizeof(long)), *result;
        data[0] = 5;
        data[1] = 2;

        result = calcPow(data);
        assert(result[0] == 25);

        data[0] = 2;
        data[1] = 5;
        result = calcPow(data);
        assert(result[0] == 32);
    }

    {
        int a = 123, result;
        result = numlen(a);
        assert(result == 3);

        a = 1;
        result = numlen(a);
        assert(result == 1);

        a = 10231;
        result = numlen(a);
        assert(result == 5);
    }

    {
        char *string = "12345678";
        int result;

        result = indexOf(string, '2');
        assert(result == 1);

        result = indexOf(string, '8');
        assert(result == 7);
    }

    {
        char *string = "FIBONACCI";
        EType result = stringToEType(string);
        assert(result == FIBONACCI);

        string = "POW";
        result = stringToEType(string);
        assert(result == POW);

        string = "BUBBLE_SORT_UINT64";
        result = stringToEType(string);
        assert(result == BUBBLE_SORT_UINT64);

        string = "STOP";
        result = stringToEType(string);
        assert(result == STOP);
    }

    {
        char string[] = "25 12 3 52 9";
        long *data = convertParameters(string);
        assert(data[0] == 25);
        assert(data[4] == 9);
    }

    {
        long *data = calloc(1, sizeof(long));
        data[0] = 123;
        TMessage *tMessage = calloc(1, sizeof(TMessage));
        tMessage->Type = 0;
        tMessage->Data = data;

        TMessage *tMessage2 = calloc(1, sizeof(TMessage));
        tMessage2->Type = 1;
        tMessage2->Data = data;

        TMessage *tMessage3 = calloc(1, sizeof(TMessage));
        tMessage3->Type = 3;
        tMessage3->Data = data;

        queueForWriting = init_queue();

        enqueue(queueForWriting, tMessage);
        enqueue(queueForWriting, tMessage2);
        enqueue(queueForWriting, tMessage3);

        pthread_create(&writerTid, &attr, writeCustom, NULL);
        pthread_join(writerTid, NULL);
        show_queue(queueForWriting);
    }

}

int main(int argc, char *argv[]) {
    pthread_attr_init(&attr);

    queueForWriting = init_queue();

    pthread_create(&writerTid, &attr, writeCustom, NULL);

    pthread_create(&readerTid, &attr, reader, NULL);
    pthread_join(readerTid, NULL);
    pthread_join(writerTid, NULL);

//    tests();
}

long *calcFib(long *data) {
    long result = recursiveFib(data[0]);
    data[0] = result;
    return data;
}

long *calcPow(long *data) {
    long result = 1;
    long number = data[0];
    for (int i = 0; i < data[1]; i++) {
        result *= number;

        if (result == 0)
            result = 1;
    }

    data = (long *) malloc(strlen(data));
    *data = result;
    return data;
}

void *doSort(long *data) {
    int n = size(data);

    for (int i = 0; i < n - 1; i++)
        for (int j = 0; j < n - i - 1; j++)
            if (data[j] > data[j + 1])
                swap(&data[j], &data[j + 1]);
}

void *reader() {
    TMessage *tMessage;

    char *buffer = calloc(1000, sizeof(char));

    do {
        fprintf(stdin, "%s", buffer);

        tMessage = createTMessage();

        if (tMessage->Type == STOP) {
            enqueue(queueForWriting, tMessage);
            break;
        }

        pthread_create(&calcTid, &attr, calc, tMessage);
        pthread_join(calcTid, NULL); // ?
    } while (1);

}

void *calc(void *data) {
    TMessage *tMessage = (TMessage *) data;

    switch (tMessage->Type) {
        case 0:
            tMessage->Data = calcFib(tMessage->Data);
            break;
        case 1:
            tMessage->Data = calcPow(tMessage->Data);
            break;
        case 2:;
            doSort(tMessage->Data);
            break;
    }

    tMessage->Size = numlen(tMessage->Data);

    enqueue(queueForWriting, tMessage);
}

void *writeCustom() {
    FILE *file = fopen(OUTPUT_FILENAME, "w");

    TMessage *tMessage;
    while (1) {
        tMessage = dequeue(queueForWriting);

        if (tMessage != NULL) {
            switch (tMessage->Type) {
                case 0:
                    fprintf(file, "%ld", tMessage->Data[0]);
                    break;
                case 1:
                    fprintf(file, "%ld", tMessage->Data[0]);
                    break;
                case 2:
                    writeMas(file, tMessage);
                    break;
                case 3:
                    fclose(file);
                    goto exit;
            }

            fprintf(file, "\n");
        }
    }
    exit:
    fclose(file);
}