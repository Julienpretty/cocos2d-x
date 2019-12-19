/****************************************************************************
 Copyright (c) 2014-2016 Chukong Technologies Inc.
 Author: Justin Graham (https://github.com/mannewalis)

 http://www.cocos2d-x.org

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 ****************************************************************************/

#pragma once

/// @cond DO_NOT_SHOW

/****************************************************************************
                                    WARNING!
     Do not use Console::log or any other methods that use NEW inside of this
     allocator. Failure to do so will result in recursive memory allocation.
 ****************************************************************************/

#include <cocos/base/allocator/CCAllocatorBase.h>
#include <cocos/base/allocator/CCAllocatorDiagnostics.h>
#include <cocos/base/allocator/CCAllocatorGlobal.h>
#include <cocos/base/allocator/CCAllocatorMacros.h>
#include <cocos/base/allocator/CCAllocatorMutex.h>
#include <cocos/platform/CCPlatformDefine.h>

#include <cstddef>
#include <cstdint>
#include <sstream>
#include <typeinfo>
#include <vector>

NS_CC_BEGIN
NS_CC_ALLOCATOR_BEGIN

// @brief define this to cause this allocator to fallback to the global allocator
// this is just for testing purposes to see if this allocator is broken.
//#define FALLBACK_TO_GLOBAL

// @brief
// Fixed sized block allocator strategy for allocating blocks
// of memory that are the same size.
// Optionally takes a page size which determines how many blocks
// are added when the allocator needs more storage.
// @param _block_size the size of the fixed block allocated by this allocator.
// @param _page_size the number of blocks to allocate when growing the free list.
// @param _alignment the alignment size in bytes of each block.
// @param locking_semantics which locking strategy to use.
template <std::size_t _block_size, std::size_t _alignment = 16, typename lock_traits = locking_semantics>
class AllocatorStrategyFixedBlock : public AllocatorBase, public lock_traits
{
public:
    static const std::size_t block_size = _block_size;
    static const std::size_t alignment = _alignment;

    AllocatorStrategyFixedBlock(const char* tag = nullptr, std::size_t pageSize = 100)
    : _list(nullptr)
    , _pages(nullptr)
    , _pageSize(pageSize)
    , _allocated(0)
    {
#if CC_ENABLE_ALLOCATOR_DIAGNOSTICS
        _highestCount = 0;
        AllocatorDiagnostics::instance()->trackAllocator(this);
        AllocatorBase::setTag(tag ? tag : typeid(AllocatorStrategyFixedBlock).name());
#endif
    }

    virtual ~AllocatorStrategyFixedBlock()
    {
#if CC_ENABLE_ALLOCATOR_DIAGNOSTICS
        AllocatorDiagnostics::instance()->untrackAllocator(this);
#endif

        do
        {
            intptr_t* page = (std::intptr_t*)_pages;
            intptr_t* next = (std::intptr_t*)*page;
            ccAllocatorGlobal.deallocate(page);
            _pages = (void*)next;
        } while (_pages);
    }

    // @brief
    // allocate a block of memory by returning the first item in the list or if empty
    // then allocate a new page of blocks, and return the first element and store the rest.
    // if _block_size does not match the requested size, then we assert.
    CC_ALLOCATOR_INLINE void* allocate(std::size_t size)
    {
        CC_ASSERT(block_size == size);
#ifdef FALLBACK_TO_GLOBAL
        return ccAllocatorGlobal.allocate(size);
#else
        lock_traits::lock();
        auto r = pop_front();
        lock_traits::unlock();
        return r;
#endif
    }

    // @brief Deallocate a block by pushing it on the head of a linked list of free blocks.
    CC_ALLOCATOR_INLINE void deallocate(void* address, std::size_t size = 0)
    {
        CC_ASSERT(0 == size || block_size == size);
#ifdef FALLBACK_TO_GLOBAL
        ccAllocatorGlobal.deallocate(address);
#else
        lock_traits::lock();
        push_front(address);
        lock_traits::unlock();
#endif
    }

    // @brief Checks allocated pages to determine whether or not a block
    // is owned by this allocator. This should be reasonably fast
    // for properly configured allocators with few large pages.
    CC_ALLOCATOR_INLINE bool owns(const void* const address)
    {
#ifdef FALLBACK_TO_GLOBAL
        return true; // since everything uses the global allocator, we can just lie and say we own this address.
#else
        lock_traits::lock();

        const std::uint8_t* const a = (const std::uint8_t* const)address;
        const std::uint8_t* p = (std::uint8_t*)_pages;
        const std::size_t pSize = pageSize();
        while (p)
        {
            if (a >= p && a < (p + pSize))
            {
                lock_traits::unlock();
                return true;
            }
            p = (std::uint8_t*)(*(std::uintptr_t*)p);
        }
        lock_traits::unlock();
        return false;
#endif
    }

#if CC_ENABLE_ALLOCATOR_DIAGNOSTICS
    std::string diagnostics() const
    {
        std::stringstream s;
        s << AllocatorBase::tag() << " initial:" << _pageSize << " count:" << _allocated << " highest:" << _highestCount << "\n";
        return s.str();
    }
    std::size_t _highestCount;
#endif

protected:
    // @brief Method to push an allocated block onto the free list.
    // No check is made that the block hasn't been already added to this allocator.
    CC_ALLOCATOR_INLINE void push_front(void* block)
    {
        CC_ASSERT(block);
        CC_ASSERT(block_size < AllocatorBase::kDefaultAlignment || 0 == ((std::intptr_t)block & (AllocatorBase::kDefaultAlignment - 1)));

#if COCOS2D_DEBUG
        // additional debug build checks
        CC_ASSERT(true == owns(block));
#endif

        if (nullptr == _list)
        {
            _list = block;
            *(std::uintptr_t*)block = 0;
        }
        else
        {
            std::uintptr_t* p = (std::uintptr_t*)(block);
            *p = (std::uintptr_t)_list;
            _list = block;
        }
        CC_ASSERT(_allocated > 0);
        --_allocated;
    }

    // @brief Method to pop a block off the free list.
    // If no blocks are available, then the list is grown by _page_size
    // Tuning of the page size is critical to getting good performance.
    // Ideally you would use a page size that is around the high water mark
    // for the number of blocks of this size being allocated.
    CC_ALLOCATOR_INLINE void* pop_front()
    {
        if (nullptr == _list)
        {
            allocatePage();
        }
        auto next = (void*)*(std::uintptr_t*)_list;
        auto block = _list;
        _list = next;
        ++_allocated;

#if CC_ENABLE_ALLOCATOR_DIAGNOSTICS
        if (_allocated > _highestCount)
            _highestCount = _allocated;
#endif
        CC_ASSERT(block_size < AllocatorBase::kDefaultAlignment || 0 == ((std::intptr_t)block & (AllocatorBase::kDefaultAlignment - 1)));
        return block;
    }

protected:
    // @brief Returns the size of a page in bytes + overhead.
    std::size_t pageSize() const { return AllocatorBase::kDefaultAlignment + AllocatorBase::nextPow2BlockSize(block_size) * _pageSize; }

    // @brief Allocates a new page from the global allocator,
    // and adds all the blocks to the free list.
    CC_ALLOCATOR_INLINE void allocatePage()
    {
        std::uint8_t* p = (std::uint8_t*)AllocatorBase::aligned(ccAllocatorGlobal.allocate(pageSize()));
        std::intptr_t* page = (std::intptr_t*)p;
        if (nullptr == _pages)
        {
            _pages = page;
            *page = 0;
        }
        else
        {
            *page = (std::intptr_t)_pages;
            _pages = page;
        }

        p += AllocatorBase::kDefaultAlignment; // step past the linked list node

        _allocated += _pageSize;
        std::size_t aligned_size = AllocatorBase::nextPow2BlockSize(block_size);
        std::uint8_t* block = (std::uint8_t*)p;
        for (unsigned int i = 0; i < _pageSize; ++i, block += aligned_size)
        {
            push_front(block);
        }
    }

protected:
    // @brief Linked list of free blocks.
    void* _list;

    // @brief Linked list of allocated pages.
    void* _pages;

    // @brief number of blocks per page.
    std::size_t _pageSize;

    // @brief Number of blocks that are currently allocated.
    std::size_t _allocated;
};

NS_CC_ALLOCATOR_END
NS_CC_END

/// @endcond