// an event handler linked list that facilitates running callbacks in the main loop after a triggered interrupt
//
// two different implementations are provided, one that uses malloc() for dynamic allocation of nodes in the list
// and another that simply manages an array EH_MAX long of event_handler structures. the CONFIG_DYN_ALLOC define
// switches between them.
//
// author:      Petre Rodan <2b4eda@subdimension.ro>
// license:     BSD

#ifndef __EVENT_HANDLER_H__
#define __EVENT_HANDLER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "proj.h"

/*!
	\brief List of possible events
	\sa eh_register()
*/

// should be re-created in local proj.h

//#define           SYS_EVH_NULL 0
//#define    SYS_EVH_TIMER0_CRR1 0x1   // timer_a0_delay_noblk_ccr1
//#define    SYS_EVH_TIMER0_CRR2 0x2   // timer_a0_delay_noblk_ccr2
//#define     SYS_EVH_TIMER0_IFG 0x4   // timer0 overflow
//#define       SYS_EVH_UART0_RX 0x8   // UART received something

/*!
	\brief linked list of event handlers
*/
struct event_handler {
    /*! interrupt handling function */
    void (*callback) (const uint32_t evid);
    /*! bitfield of triggers for which the function should be run */
    uint32_t evid;
#ifdef CONFIG_DYN_ALLOC
    /*! pointer to the next node in the list */
    struct event_handler *next;
#endif
};

/*!
	\brief get pointer to the event handler structure
	\details get pointer to the head of the event handler linked list
	\sa event_handler
*/
struct event_handler * event_handler_getp(void);

/*!
	\brief initialize the array of event handler structures
    \details zeroes out the array of the event handler structs. only needed for the non-malloc implementation
	\sa event_handler, eh_register
*/
void eh_init(void);

/*!
	\brief register a new event
	\details add a node to the event handler linked list
	\sa event_handler, eh_init, eh_unregister_callback, eh_unregister_event
*/
uint8_t eh_register(
        // interrupt handling function
        void (*callback) (const uint32_t evid),
        // flag on which to trigger
        const uint32_t evid
    );

/*!
	\brief unregister all nodes from the event handler list based on callback function
    \details find all the nodes with a particular callback and remove them from the linked list
	\sa event_handler, eh_register
*/
uint8_t eh_unregister_callback(void (*callback) (const uint32_t evid));

/*!
	\brief unregister all nodes from the event handler list based on the event
    \details find all the nodes with a particular event and remove them from the linked list
	\sa event_handler, eh_register
*/
uint8_t eh_unregister_event(const uint32_t evid);

/*!
	\brief run the callback functions registered for the event
    \details find all the nodes with a particular event and run their callback function
	\sa event_handler, eh_register
*/
void eh_exec(const uint32_t event);

#ifndef CONFIG_DYN_ALLOC

#ifndef EH_MAX
#define EH_MAX  8 // event handler array size
#endif

#endif

#ifdef __cplusplus
}
#endif

#endif
