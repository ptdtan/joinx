#pragma once

#include "common/UnsortedDataError.hpp"

#include <boost/format.hpp>
#include <cstring>
#include <list>

// This class is capable of performing intersection as well as symmetric
// difference.
template<typename StreamTypeA, typename StreamTypeB, typename CollectorType>
class IntersectFull {
public: // types and data
    // value types
    typedef typename StreamTypeA::ValueType TypeA;
    typedef typename StreamTypeB::ValueType TypeB;

    // cache types
    struct CacheEntry {
        CacheEntry(const TypeB& v) : value(v), hit(false) {}
        CacheEntry(const TypeB& v, bool hit) : value(v), hit(hit) {}

        TypeB value;
        bool hit;
    };

    typedef typename std::list<CacheEntry> CacheType;
    typedef typename CacheType::iterator CacheIterator;

    // compare results
    enum Compare {
        BEFORE,
        INTERSECT,
        AFTER
    };


public: // code
    IntersectFull(StreamTypeA& a, StreamTypeB& b, CollectorType& rc, bool adjacentInsertions = false)
        : _a(a) , _b(b), _rc(rc), _adjacentInsertions(adjacentInsertions)
    {
    }

    virtual ~IntersectFull() {}

    template<typename TA, typename TB>
    Compare compare(const TA& a, const TB& b) const {
        if (_adjacentInsertions)
            return compareWithAdjacentInsertions(a, b);

        int rv = strverscmp(a.chrom().c_str(), b.chrom().c_str());
        if (rv < 0)
            return BEFORE;
        if (rv > 0)
            return AFTER;

        // to handle identical insertions
        if (a.start() == a.stop() && b.start() == b.stop() && a.start() == b.start())
            return INTERSECT;

        if (a.stop() <= b.start())
            return BEFORE;
        if (b.stop() <= a.start())
            return AFTER;

        return INTERSECT;
    }

    template<typename TA, typename TB>
    Compare compareWithAdjacentInsertions(const TA& a, const TB& b) const {
        int rv = strverscmp(a.chrom().c_str(), b.chrom().c_str());
        if (rv < 0)
            return BEFORE;
        if (rv > 0)
            return AFTER;

        // to handle adjacent/exact match insertions!
        if ((containsInsertions(a) && (a.stop() == b.start() || a.start() == b.stop())) ||
            (containsInsertions(b) && (b.stop() == a.start() || b.start() == a.stop())) )
        {
            return INTERSECT;
        }

        if (a.stop() <= b.start())
            return BEFORE;
        if (b.stop() <= a.start())
            return AFTER;

        return INTERSECT;
    }

    bool eof() const {
        return _a.eof() || _b.eof();
    }

    bool checkCache(const TypeA& valueA) {
        bool rv = false;
        auto iter = _cache.begin();
        while (iter != _cache.end()) {
            Compare cmp = compare(valueA, iter->value);
            if (cmp == BEFORE) {
                break;
            } else if (cmp == AFTER) {
                auto toRemove = iter;
                ++iter;
                cacheRemove(toRemove);
                continue;
            }

            bool isHit = _rc.hit(valueA, iter->value);
            rv |= isHit;
            iter->hit |= isHit;
            ++iter;
        }
        return rv;
    }

    template<typename T, typename S>
    bool advanceSorted(S& stream, T& value) {
        using boost::format;
        T* peek = NULL;
        if (stream.peek(&peek) && compare(*peek, value) == BEFORE)
            throw UnsortedDataError(str(format("Unsorted data found in stream %1%\n'%2%' follows '%3%'") %stream.name() %peek->toString() %value.toString()));
        return stream.next(value);
    }

    void execute() {
        TypeA valueA;
        TypeB valueB;
        while (!_a.eof() && advanceSorted(_a, valueA)) {
            // burn entries from the cache
            while (!_cache.empty() && compare(valueA, _cache.front().value) == AFTER)
                popCache();

            // look ahead and burn entries from the input file
            TypeB* peek = NULL;
            if (_cache.empty()) {
                while (!_b.eof() && _b.peek(&peek) && compare(valueA, *peek) == AFTER) {
                    advanceSorted(_b, valueB);
                    if (_rc.wantMissB())
                        _rc.missB(valueB);
                }
            }

            bool hitA = checkCache(valueA);
            while (!_b.eof() && advanceSorted(_b, valueB)) {
                Compare cmp = compare(valueA, valueB);
                if (cmp == BEFORE) {
                    cache(valueB, false);
                    break;
                } else if (cmp == AFTER) {
                    if (_rc.wantMissB())
                        _rc.missB(valueB);
                    continue;
                }

                bool rv = _rc.hit(valueA, valueB);
                hitA |= rv;
                cache(valueB, rv);
            }

            if (!hitA && _rc.wantMissA()) {
                _rc.missA(valueA);
            }
        }
        while (_rc.wantMissB() && !_cache.empty())
            popCache();
        while (_rc.wantMissB() && !_b.eof() && advanceSorted(_b, valueB))
            _rc.missB(valueB);
    }

    void cache(const TypeB& valueB, bool hit) {
        _cache.push_back(CacheEntry(valueB, hit));
    }

    void cacheRemove(const CacheIterator& iter) {
        if (!iter->hit && _rc.wantMissB())
            _rc.missB(iter->value);
        _cache.erase(iter);
    }

    void popCache() {
        cacheRemove(_cache.begin());
    }

protected:
    StreamTypeA& _a;
    StreamTypeB& _b;
    CollectorType& _rc;
    CacheType _cache;
    bool _adjacentInsertions;
};

template<typename StreamTypeA, typename StreamTypeB, typename OutType>
IntersectFull<StreamTypeA, StreamTypeB, OutType>
makeFullIntersector(StreamTypeA& sa, StreamTypeB& sb, OutType& out, bool adjacentInsertions = false) {
    return IntersectFull<StreamTypeA, StreamTypeB, OutType>(sa, sb, out, adjacentInsertions);
}

