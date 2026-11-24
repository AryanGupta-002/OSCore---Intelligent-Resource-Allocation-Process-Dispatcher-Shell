// Code by Bibham Bibhuti
#include "resource_manager.h"

// Yahan pr hum system ke sare available resources ko initialize karenge
// jaise ki printers, scanners, modems aur CDs.

void initialize_resources(
    ResourceManager *resources,
    int printers,
    int scanners,
    int modems,
    int cds
)
{
    // Pahle check kar rhe hai ki resource manager valid hai ya nahi
    // Agar NULL hai toh aage execute karne ka koi matlab nahi hai.
    if (resources == NULL) {
        return;
    }

    // Yahan pr system ke total available resources ki quantity set kar rhe hai
    // Agar future mein koi new resource add karna ho toh uski initial value bhi yahan set karenge.
    resources->printers_available = printers;
    resources->scanners_available = scanners;
    resources->modems_available = modems;
    resources->cds_available = cds;
}


// Yahan pr hum check kar rhe hai ki kisi process ko uske required resources
// allocate kiye ja sakte hai ya nahi.
// Return 1 ka matlab resources available hai aur 0 ka matlab available nahi hai idhar....

int can_allocate_resources(
    const Process *p,
    const ResourceManager *resources
)
{
    // Check kar rhe hai ki process and resource manager dono valid hai ya nahi
    if (p == NULL || resources == NULL) {
        return 0;
    }

    // Negative resource requirements valid nahi hoti hai
    // Isliye agar kisi bhi resource ki requirement negative hai toh process reject ho jayega.
    if (p->printers_required < 0 ||
        p->scanners_required < 0 ||
        p->modems_required < 0 ||
        p->cds_required < 0) {
        return 0;
    }

    // Yahan pr process ki required quantity ko currently available resources se compare kar rhe hai
    // AND (&&) use kiya hai kyunki allocation ke liye sari conditions true honi chahiye.
    // Agar kisi ek resource ki bhi quantity available nahi hai toh 0 return hoga.
    return (
        p->printers_required <= resources->printers_available &&
        p->scanners_required <= resources->scanners_available &&
        p->modems_required <= resources->modems_available &&
        p->cds_required <= resources->cds_available
    );
}


// Is function ka use process ko resources actually allocate karne ke liye ho raha hai
// Pahle availability check hogi aur uske baad resources allocate honge.

int allocate_resources(
    const Process *p,
    ResourceManager *resources
)
{
    // Pahle check kar rhe hai ki process ko required resources mil sakte hai ya nahi
    // Agar resources available nahi hai toh allocation yahin stop ho jayega.
    if (!can_allocate_resources(p, resources)) {
        return 0;
    }

    // Yahan pr allocated resources ko total available resources mein se subtract kar rhe hai
    // Isse allocation ke baad remaining resources ki updated quantity mil jayegi.
    resources->printers_available -= p->printers_required;
    resources->scanners_available -= p->scanners_required;
    resources->modems_available -= p->modems_required;
    resources->cds_available -= p->cds_required;

    // Agar allocation successfully ho gaya toh 1 return karenge.
    return 1;
}


// Jab process ka execution complete ho jata hai tab uske allocated resources
// ko system mein wapas release karne ke liye ye function use hota hai.

void release_resources(
    const Process *p,
    ResourceManager *resources
)
{
    // Check kar rhe hai ki process aur resource manager valid hai ya nahi
    if (p == NULL || resources == NULL) {
        return;
    }

    // Process ko jo resources allocate kiye gaye the unhe wapas available resources mein add kar rhe hai
    // Isse dusre processes bhi in resources ka use kar sakenge.
    resources->printers_available += p->printers_required;
    resources->scanners_available += p->scanners_required;
    resources->modems_available += p->modems_required;
    resources->cds_available += p->cds_required;
}
