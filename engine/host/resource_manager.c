
#include "resource_manager.h"

void initialize_resources(
    ResourceManager *resources,
    int printers,
    int scanners,
    int modems,
    int cds
)
{
    if (resources == NULL) {
        return;
    }

    resources->printers_available = printers;
    resources->scanners_available = scanners;
    resources->modems_available = modems;
    resources->cds_available = cds;
}

int can_allocate_resources(
    const Process *p,
    const ResourceManager *resources
)
{
    if (p == NULL || resources == NULL) {
        return 0;
    }

    /* Reject invalid resource requirements */
    if (p->printers_required < 0 ||
        p->scanners_required < 0 ||
        p->modems_required < 0 ||
        p->cds_required < 0) {
        return 0;
    }

    return (
        p->printers_required <= resources->printers_available &&
        p->scanners_required <= resources->scanners_available &&
        p->modems_required <= resources->modems_available &&
        p->cds_required <= resources->cds_available
    );
}

int allocate_resources(
    const Process *p,
    ResourceManager *resources
)
{
    if (!can_allocate_resources(p, resources)) {
        return 0;
    }

    resources->printers_available -= p->printers_required;
    resources->scanners_available -= p->scanners_required;
    resources->modems_available -= p->modems_required;
    resources->cds_available -= p->cds_required;

    return 1;
}

void release_resources(
    const Process *p,
    ResourceManager *resources
)
{
    if (p == NULL || resources == NULL) {
        return;
    }

    resources->printers_available += p->printers_required;
    resources->scanners_available += p->scanners_required;
    resources->modems_available += p->modems_required;
    resources->cds_available += p->cds_required;
}
