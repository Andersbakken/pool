#include <stdlib.h>
#include <assert.h>
#include <unordered_set>
#include <type_traits>
#include <string.h>
#include <memory>

template <typename T, size_t Count>
class Pool
{
public:
    Pool()
    {
        memset(begin(), 0, Count * sizeof(T));
        size_t idx = 0;
        const T *e = end();
        for (T *t = begin(); t < e; ++t, ++idx) {
            int nextFree = idx + 1;
            if (nextFree == Count)
                nextFree = -1;
            memcpy(t, &nextFree, sizeof(nextFree));
        }
    }
    ~Pool()
    {
#ifndef NDEBUG
        size_t freeCount = 0;
        while (mFirstFree != -1) {
            int tmp;
            memcpy(&tmp, begin() + mFirstFree, sizeof(mFirstFree));
            mFirstFree = tmp;
            ++freeCount;
        }
        assert(freeCount == Count);
#endif
    }

    constexpr size_t size() const { return Count; }
    template <typename ...Args>
    T *allocate(Args &&...args)
    {
        if (mFirstFree == -1) {
            return new T(args...);
        }
        int nextFree;
        memcpy(&nextFree, begin() + mFirstFree, sizeof(int));
        T *ret = new (begin() + mFirstFree) T(std::forward<Args>(args)...);
        mFirstFree = nextFree;
        return ret;
    }

    void free(T *t)
    {
        if (t < begin() || t >= end()) {
            delete t;
            return;
        }

        t->~T();
        memcpy(t, &mFirstFree, sizeof(mFirstFree));
        mFirstFree = t - begin();
    }
    size_t allocated() const { return mAllocated; }
private:
    T *begin() { return reinterpret_cast<T *>(mData); }
    T *end() { return reinterpret_cast<T *>(mData) + Count; }
    typedef typename std::aligned_storage<sizeof(T), alignof(T)>::type *PointerType;
    size_t mAllocated;
    int mFirstFree { 0 };
    typename std::aligned_storage<sizeof(T), alignof(T)>::type mData[Count];
};

struct Chunk
{
    Chunk()
        : idx(-1)
    {}

    Chunk(int i)
        : idx(i)
    {}

    ~Chunk()
    {
    }
    int idx;
};

int main(int argc, char **argv)
{
    Pool<Chunk, 10> pool;
    std::unordered_set<Chunk *> chunks;
    for (size_t i=0; i<pool.size(); ++i) {
        chunks.insert(pool.allocate(i));
    }
    size_t deads = 0;

    for (size_t i=pool.size(); i<100000 + pool.size(); ++i) {
        if (rand() % 2 == 0 && !chunks.empty()) {
            Chunk *ch = *chunks.begin();
            deads += ch->idx;
            pool.free(ch);
            chunks.erase(chunks.begin());
        } else {
            chunks.insert(pool.allocate(i));

        }
    }
    for (Chunk *chunk : chunks) {
        pool.free(chunk);
    }

    return 0;
}
