// Code by Bibham Bibhuti
#ifndef RESOURCE_MANAGER_H
#define RESOURCE_MANAGER_H

#include <stddef.h>
#include "../include/process.h"

// Yahan hum system mein currently available hardware resources ka count store karenge...
typedef struct {
    int printers_available;  // Currently kitne printers available hai...
    int scanners_available;  // Currently kitne scanners available hai...
    int modems_available;    // Currently kitne modems available hai...
    int cds_available;       // Currently kitne CDs available hai...
} ResourceManager;


// Yahan hum starting mein total available resources initialize karenge...
// Agar future mein naye resources add karne hain toh structure aur parameters dono update karne honge...
void initialize_resources(
    ResourceManager **resources,
    int printers,
    int scanners,
    int modems,
    int cds
);


// Idhar hum check karenge ki kisi process ko uski required resources mil sakti hai ya nahi...
// Agar saare required resources available hain toh 1 return hoga otherwise 0...
int can_allocate_resources(
    const Process *p,
    const ResourceManager *resources
);


// Agar process ki saari requirements fulfill ho sakti hain toh usko resources allocate kar denge...
// Allocation successful hone par 1 aur resources available na hone par 0 return hoga...
int allocate_resources(
    const Process *p,
    ResourceManager *resources
);


// Jab process apna execution complete kar lega toh uske allocated resources wapas release kar denge...
// Isse released resources dobara available ho jayenge aur dusre processes unhe use kar sakenge...
void release_resources(
    const Process *p,
    ResourceManager *resources
);

#endif
