#include <iostream>
#include "memory_pool.h"
using namespace std;

int main() {
    void* p[20];
    for (int i = 0; i < 20; i++) {
        p[i] = alloc::allocate(32);
        cout << "alloc " << i << " -> " << p[i] << endl;
    }

    for (int i = 0; i < 20; i++) {
        alloc::deallocate(p[i], 32);
        cout << "free " << i << endl;
    }

    cout << "All memory deallocated." << endl;
    return 0;
}