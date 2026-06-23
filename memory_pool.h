#pragma once
#include <cstdlib>
#include <mutex>
#include <cstddef>

const int MAX_BYTES = 128;
const int ALLOC_COUNT = 16;
const int ALLOC_UNIT = 8;

template<bool threads, int inst>
class _default_alloc {
private:
    union obj {
        obj* free_list_link;
    };

    struct mem_chunk {
        char* addr;
        mem_chunk* next;
    };

    static obj* volatile free_list[ALLOC_COUNT];
    static mem_chunk* chunk_list[ALLOC_COUNT];
    static size_t _use_count[ALLOC_COUNT];  // ����ʹ�ü���
    static std::mutex _mutex;

    static size_t ROUND_UP(size_t bytes) {
        return (bytes + ALLOC_UNIT - 1) & ~(ALLOC_UNIT - 1);
    }

    static size_t INDEX(size_t bytes) {
        return (bytes + ALLOC_UNIT - 1) / ALLOC_UNIT - 1;
    }

    static void* refill(size_t n);
    static char* chunk_alloc(size_t n, size_t& n_objs, size_t idx);
    static void shrink_to_os(size_t idx);

public:
    static void* allocate(size_t n);
    static void deallocate(void* p, size_t n);
};

template<bool threads, int inst>
typename _default_alloc<threads, inst>::obj* volatile
_default_alloc<threads, inst>::free_list[ALLOC_COUNT] = { nullptr };

template<bool threads, int inst>
typename _default_alloc<threads, inst>::mem_chunk*
_default_alloc<threads, inst>::chunk_list[ALLOC_COUNT] = { nullptr };

template<bool threads, int inst>
size_t _default_alloc<threads, inst>::_use_count[ALLOC_COUNT] = { 0 };

template<bool threads, int inst>
std::mutex _default_alloc<threads, inst>::_mutex;


template<bool threads, int inst>
void* _default_alloc<threads, inst>::allocate(size_t n) {
    if (n == 0) n = 1;
    if (n > MAX_BYTES) return malloc(n);

    size_t idx = INDEX(n);
    if (threads) _mutex.lock();

    obj* my_free = free_list[idx];
    if (!my_free) {
        void* r = refill(ROUND_UP(n));
        _use_count[idx]++;
        if (threads) _mutex.unlock();
        return r;
    }

    free_list[idx] = my_free->free_list_link;
    _use_count[idx]++;

    if (threads) _mutex.unlock();
    return my_free;
}


template<bool threads, int inst>
void _default_alloc<threads, inst>::deallocate(void* p, size_t n) {
    if (!p) return;
    if (n == 0) return;
    if (n > MAX_BYTES) { free(p); return; }

    size_t idx = INDEX(n);
    obj* q = (obj*)p;

    if (threads) _mutex.lock();

    q->free_list_link = free_list[idx];
    free_list[idx] = q;

    _use_count[idx]--;

    if (_use_count[idx] == 0) {
        shrink_to_os(idx);
    }

    if (threads) _mutex.unlock();
}

//  refill�������ڴ��
template<bool threads, int inst>
void* _default_alloc<threads, inst>::refill(size_t n) {
    size_t nobjs = 20;
    char* c = chunk_alloc(n, nobjs, INDEX(n));

    if (nobjs == 1) return c;

    obj* cur = (obj*)c;
    obj* next = (obj*)((char*)cur + n);
    free_list[INDEX(n)] = next;

    for (size_t i = 1;; i++) {
        cur = next;
        next = (obj*)((char*)next + n);
        if (i == nobjs - 1) {
            cur->free_list_link = nullptr;
            break;
        }
        cur->free_list_link = next;
    }
    return c;
}


template<bool threads, int inst>
char* _default_alloc<threads, inst>::chunk_alloc(size_t n, size_t& n_objs, size_t idx) {
    size_t total = n * n_objs;
    void* p = malloc(total);
    if (!p) return nullptr;

    mem_chunk* mc = (mem_chunk*)malloc(sizeof(mem_chunk));
    if (!mc) {
        free(p);
        return nullptr;
    }
    mc->addr = (char*)p;
    mc->next = chunk_list[idx];
    chunk_list[idx] = mc;

    return (char*)p;
}

template<bool threads, int inst>
void _default_alloc<threads, inst>::shrink_to_os(size_t idx) {
    // 当没有使用中的对象时，安全地释放所有 chunk 并清空 freelist，
    // 否则保留当前状态。调用处在持锁状态下，故此处无需再次加锁。
    if (_use_count[idx] != 0) return;

    mem_chunk* cur = chunk_list[idx];
    while (cur) {
        mem_chunk* temp = cur->next;
        free(cur->addr);
        free(cur);
        cur = temp;
    }
    chunk_list[idx] = nullptr;
    free_list[idx] = nullptr;
}

typedef _default_alloc<true, 0> alloc;