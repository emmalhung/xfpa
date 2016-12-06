#ifndef MONITOR_H
#define MONITOR_H

#include "event_queue.h"

extern LOGICAL read_monitor_config    (xmlNodePtr root, STRING logfile);
extern void    process_monitor_events (void);
extern void    set_monitor_watch      (int);
extern void    add_to_monitor_list    (queue_entry_t);

#endif
