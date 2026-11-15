
#ifndef RESOURCE_MANAGER_H
#define RESOURCE_MANAGER_H
#include <stddef.h>
#include "../include/process.h"

/* Tracks currently available hardware resources */
typedef struct {
    int printers_available;
    int scanners_available;
    int modems_available;
    int cds_available;
} ResourceManager;

/* Reset all resources to their initial capacity */
void initialize_resources(
    ResourceManager *resources,
    int printers,
    int scanners,
    int modems,
    int cds
);

/* Check whether a process can obtain its required resources */
int can_allocate_resources(
    const Process *p,
    const ResourceManager *resources
);

/* Allocate resources if all requirements can be met */
int allocate_resources(
    const Process *p,
    ResourceManager *resources
);

/* Return a process's resources after execution */
void release_resources(
    const Process *p,
    ResourceManager *resources
);

#endif
