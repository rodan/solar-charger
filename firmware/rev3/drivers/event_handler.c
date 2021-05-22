
// an event handler linked list that facilitates running callbacks in the main loop after a triggered interrupt
//
// two different implementations are provided, one that uses malloc() for dynamic allocation of nodes in the list
// and another that simply manages an array EH_MAX long of event_handler structures. the CONFIG_DYN_ALLOC define
// switches between them.
//
// author:      Petre Rodan <2b4eda@subdimension.ro>
// license:     BSD

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include "event_handler.h"

#ifdef CONFIG_DYN_ALLOC
static struct event_handler *evh;
#else
static struct event_handler evh[EH_MAX];
int8_t event_trail;             // the last populated event struct in the evh array
#endif

struct event_handler *event_handler_getp(void)
{
    return evh;
}

#ifdef CONFIG_DYN_ALLOC

void eh_init(void)
{

}

uint8_t eh_register(void (*callback) (const uint32_t evid), const uint32_t evid)
{
    struct event_handler **p = &evh;

    while (*p) {
        p = &(*p)->next;
    }

    *p = (struct event_handler *)malloc(sizeof(struct event_handler));

    if (*p == NULL) {
        return EXIT_FAILURE;
    }
    (*p)->next = NULL;
    (*p)->callback = callback;
    (*p)->evid = evid;
    return EXIT_SUCCESS;
}

uint8_t eh_unregister_callback(void (*callback) (const uint32_t evid))
{
    struct event_handler *p = evh, *pp = NULL, *rm;
    uint8_t ret = EXIT_FAILURE;

    while (p) {
        rm = NULL;
        if (p->callback == callback) {
            if (!pp) {
                evh = p->next;
            } else {
                pp->next = p->next;
            }
            rm = p;
        } else {
            pp = p;
        }
        p = p->next;

        if (rm) {
            free(rm);
            ret = EXIT_SUCCESS;
        }
    }

    return ret;
}

uint8_t eh_unregister_event(const uint32_t evid)
{
    struct event_handler *p = evh, *pp = NULL, *rm;
    uint8_t ret = EXIT_FAILURE;

    while (p) {
        rm = NULL;
        if (p->evid == evid) {
            if (!pp) {
                evh = p->next;
            } else {
                pp->next = p->next;
            }
            rm = p;
        } else {
            pp = p;
        }
        p = p->next;

        if (rm) {
            free(rm);
            ret = EXIT_SUCCESS;
        }
    }

    return ret;
}

void eh_exec(const uint32_t event)
{
    struct event_handler *p = evh;

    while (p) {
        // run the callback function if it's registered for the current event
        if (event & p->evid) {
            p->callback(event);
        }
        p = p->next;
    }
}

#else
// same, but don't use malloc()

void eh_init(void)
{
    event_trail = -1;
    memset(evh, 0, sizeof(struct event_handler) * EH_MAX);
}

uint8_t eh_register(void (*callback) (const uint32_t evid), const uint32_t evid)
{
    if (event_trail > EH_MAX - 2) {
        return EXIT_FAILURE;
    }
    event_trail++;
    evh[event_trail].callback = callback;
    evh[event_trail].evid = evid;

    return EXIT_SUCCESS;
}

uint8_t eh_unregister_callback(void (*callback) (const uint32_t evid))
{
    int8_t c;
    uint8_t ret = EXIT_FAILURE;

    for (c = event_trail; c > -1; c--) {
        if (evh[c].callback == callback) {
            if (c != event_trail) {
                evh[c].callback = evh[event_trail].callback;
                evh[c].evid = evh[event_trail].evid;
            }
            evh[event_trail].callback = NULL;
            evh[event_trail].evid = 0;
            event_trail--;
            ret = EXIT_SUCCESS;
        }
    }

    return ret;
}

uint8_t eh_unregister_event(const uint32_t evid)
{
    int8_t c;
    uint8_t ret = EXIT_FAILURE;

    for (c = event_trail; c > -1; c--) {
        if (evh[c].evid == evid) {
            if (c != event_trail) {
                evh[c].callback = evh[event_trail].callback;
                evh[c].evid = evh[event_trail].evid;
            }
            evh[event_trail].callback = NULL;
            evh[event_trail].evid = 0;
            event_trail--;
            ret = EXIT_SUCCESS;
        }
    }

    return ret;
}

void eh_exec(const uint32_t event)
{
    int8_t c;

    for (c = event_trail; c > -1; c--) {
        if (event & evh[c].evid) {
            // run the callback function if it's registered for the current event
            evh[c].callback(event);
        }
    }
}

#endif

