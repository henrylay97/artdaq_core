// This file (QuickVec.hh) was created by Ron Rechenmacher <ron@fnal.gov> on
// Sep  3, 2014. "TERMS AND CONDITIONS" governing this file are in the README
// or COPYING file. If you do not have such a file, one can be obtained by
// contacting Ron or Fermi Lab in Batavia IL, 60510, phone: 630-840-3000.
// $RCSfile: QuickVec.hh,v $
// rev="$Revision: 1.8 $$Date: 2014/09/05 19:21:11 $";
#ifndef QuickVec_hh
#define QuickVec_hh

//extern "C" {
//#include <stdint.h>
//}

#include <string.h>  // memcpy
//#include <strings.h>		// bzero
//#include <stdlib.h>		// posix_memalign
#include <cstddef>  // ptrdiff_t
//#include <utility>		// std::swap
//#include <memory>		// unique_ptr
/** \cond  */
#include <cassert>
#include <cmath>
#include <vector>
/** \endcond */

//#include "trace.h"		// TRACE
#ifndef TRACEN
#define TRACEN(nam, lvl, ...)
#define UNDEF_TRACE_AT_END
#endif

#define QV_ALIGN 512

/**
 * \brief Allocates aligned memory for the QuickVec
 * \param boundary The alignment boundary
 * \param size The size of memory to allocate
 * \return Pointer to allocated memory.
 */
static inline void* QV_MEMALIGN(size_t boundary, size_t size)
{
	void* retadr;
	posix_memalign(&retadr, boundary, size);
	return retadr;
}

#ifndef QUICKVEC_DO_TEMPLATE
#define QUICKVEC_DO_TEMPLATE 1
#endif

#undef NOT_OLD_CXXSTD
#if !defined(__GCCXML__) && defined(__GXX_EXPERIMENTAL_CXX0X__)
#define NOT_OLD_CXXSTD 1
#endif

#if QUICKVEC_DO_TEMPLATE == 0
#ifndef QUICKVEC_TT
#define QUICKVEC_TT unsigned long long
#endif
#define TT_ QUICKVEC_TT
#define QUICKVEC_TEMPLATE
#define QUICKVEC QuickVec
#define QUICKVEC_TN QuickVec
#define QUICKVEC_VERSION
#else
#define QUICKVEC_TEMPLATE template<typename TT_>
#define QUICKVEC QuickVec<TT_>
#define QUICKVEC_TN typename QuickVec<TT_>
#define QUICKVEC_VERSION                                                      \
	/**                                                                       \
	 * \brief Returns the current version of the template code                \
	 * \return The current version of the QuickVec                            \
	 *                                                                        \
	 * Class_Version() MUST be updated every time private member data change. \
	 */                                                                       \
	static short Class_Version() { return 5; }  // proper version for templates
#endif

namespace artdaq {

/**
 * \brief A QuickVec behaves like a std::vector, but does no initialization of its data, making it faster at
 * the cost of having to ensure that uninitialized data is not read.
 * \tparam TT_ The data type stored in the QuickVec
 */
QUICKVEC_TEMPLATE
struct QuickVec
{
	typedef TT_* iterator;               ///< Iterator is pointer-to-member type
	typedef const TT_* const_iterator;   ///< const_iterator is const-pointer-to-member type
	typedef TT_& reference;              ///< reference is reference-to-member tpye
	typedef const TT_& const_reference;  ///< const_reference is const-reference-to-member type
	typedef TT_ value_type;              ///< value_type is member type
	typedef ptrdiff_t difference_type;   ///< difference_type is ptrdiff_t
	typedef size_t size_type;            ///< size_type is size_t

	/**
	 * \brief Allocates a QuickVec object, doing no initialization of allocated memory
	 * \param sz Size of QuickVec object to allocate
	 */
	QuickVec(size_t sz);

	/**
	 * \brief Allocates a QuickVec object, initializing each element to the given value
	 * \param sz Size of QuickVec object to allocate
	 * \param val Value with which to initialize elements
	 */
	QuickVec(size_t sz, TT_ val);

	/**
	 * \brief Destructor calls free on data.
	 */
	virtual ~QuickVec() noexcept;

	/**
	 * \brief Copies the contents of a std::vector into a new QuickVec object
	 * \param other The vector to copy
	 */
	QuickVec(std::vector<TT_>& other)
	    : size_(other.size())
	    , data_((TT_*)QV_MEMALIGN(QV_ALIGN, other.capacity() * sizeof(TT_)))
	    , capacity_(other.capacity())
	{
		TRACEN("QuickVec", 10, "QuickVec std::vector ctor b4 memcpy this=%p data_=%p &other[0]=%p size_=%d other.size()=%d", (void*)this, (void*)data_, (void*)&other[0], size_, other.size());
		memcpy(data_, (void*)&other[0], size_ * sizeof(TT_));
	}

	/**
	 * \brief Sets the size to 0. QuickVec does not reinitialize memory, so no further action will be taken.
	 */
	void clear() { size_ = 0; }

	//: size_(other.size_), data_(new TT_[other.capacity_]), capacity_(other.capacity_)
	/**
	 * \brief Copy Constructor
	 * \param other QuickVec to copy
	 */
	QuickVec(const QuickVec& other)  //= delete; // non construction-copyable
	    : size_(other.size_)
	    , data_((TT_*)QV_MEMALIGN(QV_ALIGN, other.capacity() * sizeof(TT_)))
	    , capacity_(other.capacity_)
	{
		TRACEN("QuickVec", 10, "QuickVec copy ctor b4 memcpy this=%p data_=%p other.data_=%p size_=%d other.size_=%d", (void*)this, (void*)data_, (void*)other.data_, size_, other.size_);
		memcpy(data_, other.data_, size_ * sizeof(TT_));
	}

	/**
	 * \brief Copy assignment operator
	 * \param other QuickVec to copy
	 * \return Reference to new QuickVec object
	 */
	QUICKVEC& operator=(const QuickVec& other)  //= delete; // non copyable
	{
		TRACEN("QuickVec", 10, "QuickVec copy assign b4 resize/memcpy this=%p data_=%p other.data_=%p size_=%d other.size_=%d", (void*)this, (void*)data_, (void*)other.data_, size_, other.size_);
		resize(other.size_);
		memcpy(data_, other.data_, size_ * sizeof(TT_));
		return *this;
	}
#if NOT_OLD_CXXSTD
	/**
	 * \brief Move Constructor
	 * \param other QuickVec to move from
	 */
	QuickVec(QuickVec&& other) noexcept  // construction-movable
	    : size_(other.size_)
	    , data_(std::move(other.data_))
	    , capacity_(other.capacity_)
	{
		TRACEN("QuickVec", 10, "QuickVec move ctor this=%p data_=%p other.data_=%p", (void*)this, (void*)data_, (void*)other.data_);
		other.data_ = nullptr;
	}

	/**
	 * \brief Move assignemnt operator
	 * \param other QuickVec to move from
	 * \return Reference to new QuickVec object
	 */
	QUICKVEC& operator=(QuickVec&& other) noexcept  // assign movable
	{
		TRACEN("QuickVec", 10, "QuickVec move assign this=%p data_=%p other.data_=%p", (void*)this, (void*)data_, (void*)other.data_);
		size_ = other.size_;
		//delete [] data_;
		free(data_);
		data_ = std::move(other.data_);
		capacity_ = other.capacity_;
		other.data_ = nullptr;
		return *this;
	}
#endif

	/**
	 * \brief Returns a reference to a given element
	 * \param idx Element to return
	 * \return Reference to element
	 */
	TT_& operator[](int idx);

	/**
	 * \brief Returns a const reference to a given element
	 * \param idx Element to return
	 * \return const reference to element
	 */
	const TT_& operator[](int idx) const;

	/**
	 * \brief Accesses the current size of the QuickVec
	 * \return The current size of the QuickVec
	 */
	size_t size() const;

	/**
	 * \brief Accesses the current capacity of the QuickVec
	 * \return The current capacity of the QuickVec
	 * 
	 * Accesses the current capcity of the QuickVec. Like a vector,
	 * the capacity of a QuickVec object is defined as the maximum size
	 * it can hold before it must reallocate more memory.
	 */
	size_t capacity() const;

	/**
	 * \brief Gets an iterator to the beginning of the QuickVec
	 * \return An iterator to the beginning of the QuickVec
	 */
	iterator begin();

	/**
	 * \brief Gets a const_iterator to the beginning of the QuickVec
	 * \return A const_iterator to the beginning of the QuickVec
	 */
	const_iterator begin() const;

	/**
	 * \brief Gets an iterator to the end of the QuickVec
	 * \return An iterator to the end of the QuickVec
	 */
	iterator end();

	/**
	 * \brief Gets a const_iterator to the end of the QuickVec
	 * \return A const_iterator to the end of the QuickVec
	 */
	const_iterator end() const;

	/**
	 * \brief Allocates memory for the QuickVec so that its capacity is at least size
	 * \param size The new capacity of the QuickVec
	 * 
	 * Allocates memory for the QuickVec so that its capacity is at least size.
	 * If the QuickVec is already at or above size in capacity, no allocation is performed.
	 */
	void reserve(size_t size);

	/**
	 * \brief Resizes the QuickVec
	 * \param size New size of the QuickVec
	 * 
	 * If size is smaller than the current size of the QuickVec, then it will change its
	 * size_ parameter (no reallocation, capacity does not change). If size is greater than
	 * the capacity of the QuickVec, a reallocation will occur.
	 */
	void resize(size_t size);

	/**
	 * \brief Resizes the QuickVec and requests additional capacity
	 * \param size New size of the QuickVec
	 * \param growthFactor Factor to use when allocating additional capacity
	 * 
	 * This method updates the size of the QuickVec.  If the new size is within the current
	 * capacity, no realloction takes place.  If not, then the reallocation reserves
	 * additional capacity as a cushion against future needs to reallocate, based
	 * on the specified growth factor.
	 */
	void resizeWithCushion(size_t size, double growthFactor = 1.3);

	/**
	 * \brief Resizes the QuickVec, initializes new elements with val
	 * \param size New size of the QuickVec
	 * \param val Value with which to initialize elements
	 */
	void resize(size_t size, TT_ val);

	/**
	 * \brief Inserts an element into the QuickVec
	 * \param position Position at which to isnert
	 * \param nn Number of copies of val to insert
	 * \param val Value to insert
	 * \return Iterator to first inserted element
	 * 
	 * Inserts an element (or copies thereof) into the QuickVec.
	 * Note that since the underlying data structure resembles a std::vector,
	 * insert operations are very inefficient!
	 */
	iterator insert(const_iterator position, size_t nn, const TT_& val);

	/**
	 * \brief Inserts a range of elements into the QuickVec
	 * \param position Position at which to insert
	 * \param first const_iterator to first element to insert
	 * \param last const_iterator to last element to insert
	 * \return Iterator to first inserted element
	 * 
	 * Inserts elements into the QuickVec.
	 * Note that since the underlying data structure resembles a std::vector,
	 * insert operations are very inefficient!
	 */
	iterator insert(const_iterator position, const_iterator first, const_iterator last);

	/**
	 * \brief Erases elements in given range from the QuickVec
	 * \param first First element to erase
	 * \param last Last element to erase
	 * \return iterator to first element after erase range
	 * 
	 * Erases elements in given range from the QuickVec.
	 * Note that since the underlying data structure resembles a std::vector,
	 * erase operations are very inefficient! (O(n))
	 */
	iterator erase(const_iterator first, const_iterator last);

	/**
	 * \brief Exchanges references to two QuickVec objects
	 * \param other Other QuickVec to swap with
	 */
	void swap(QuickVec& other) noexcept;

	/**
	 * \brief Adds a value to the QuickVec, resizing if necessary (adds 10% capacity)
	 * \param val Value to add to the QuickVec
	 */
	void push_back(const value_type& val);

	QUICKVEC_VERSION

private:
	// Root needs the size_ member first. It must be of type int.
	// Root then needs the [size_] comment after data_.
	// Note: NO SPACE between "//" and "[size_]"
	unsigned size_;
	TT_* data_;  //[size_]
	unsigned capacity_;
};

QUICKVEC_TEMPLATE
inline QUICKVEC::QuickVec(size_t sz)
    : size_(sz)
    , data_((TT_*)QV_MEMALIGN(QV_ALIGN, sz * sizeof(TT_)))
    , capacity_(sz)
{
	TRACEN("QuickVec", 15, "QuickVec %p ctor sz=%d data_=%p", (void*)this, size_, (void*)data_);
}

QUICKVEC_TEMPLATE
inline QUICKVEC::QuickVec(size_t sz, TT_ val)
    : size_(sz)
    , data_((TT_*)QV_MEMALIGN(QV_ALIGN, sz * sizeof(TT_)))
    , capacity_(sz)
{
	TRACEN("QuickVec", 15, "QuickVec %p ctor sz=%d/v data_=%p", (void*)this, size_, (void*)data_);
	for (iterator ii = begin(); ii != end(); ++ii) *ii = val;
	//bzero( &data_[0], (sz<4)?(sz*sizeof(TT_)):(4*sizeof(TT_)) );
}

QUICKVEC_TEMPLATE
inline QUICKVEC::~QuickVec() noexcept
{
	TRACEN("QuickVec", 15, "QuickVec %p dtor start data_=%p size_=%d", (void*)this, (void*)data_, size_);
	free(data_);
	TRACEN("QuickVec", 15, "QuickVec %p dtor return", (void*)this);
}

QUICKVEC_TEMPLATE
inline TT_& QUICKVEC::operator[](int idx)
{
	assert(idx < (int)size_);
	return data_[idx];
}

QUICKVEC_TEMPLATE
inline const TT_& QUICKVEC::operator[](int idx) const
{
	assert(idx < (int)size_);
	return data_[idx];
}

QUICKVEC_TEMPLATE
inline size_t QUICKVEC::size() const { return size_; }

QUICKVEC_TEMPLATE
inline size_t QUICKVEC::capacity() const { return capacity_; }

QUICKVEC_TEMPLATE
inline QUICKVEC_TN::iterator QUICKVEC::begin() { return iterator(data_); }

QUICKVEC_TEMPLATE
inline QUICKVEC_TN::const_iterator QUICKVEC::begin() const { return iterator(data_); }

QUICKVEC_TEMPLATE
inline QUICKVEC_TN::iterator QUICKVEC::end() { return iterator(data_ + size_); }

QUICKVEC_TEMPLATE
inline QUICKVEC_TN::const_iterator QUICKVEC::end() const { return const_iterator(data_ + size_); }

QUICKVEC_TEMPLATE
inline void QUICKVEC::reserve(size_t size)
{
	if (size > capacity_)  // reallocation if true
	{
		TT_* old = data_;
		//data_ = new TT_[size];
		data_ = (TT_*)QV_MEMALIGN(QV_ALIGN, size * sizeof(TT_));
		memcpy(data_, old, size_ * sizeof(TT_));
		TRACEN("QuickVec", 13, "QUICKVEC::reserve after memcpy this=%p old=%p data_=%p capacity=%d", (void*)this, (void*)old, (void*)data_, (int)size);
		free(old);
		capacity_ = size;
	}
}

QUICKVEC_TEMPLATE
inline void QUICKVEC::resize(size_t size)
{
	if (size < size_)
		size_ = size;  // decrease
	else if (size <= capacity_)
		size_ = size;
	else  // increase/reallocate
	{
		TT_* old = data_;
		data_ = (TT_*)QV_MEMALIGN(QV_ALIGN, size * sizeof(TT_));
		memcpy(data_, old, size_ * sizeof(TT_));
		TRACEN("QuickVec", 13, "QUICKVEC::resize after memcpy this=%p old=%p data_=%p size=%d", (void*)this, (void*)old, (void*)data_, (int)size);
		free(old);
		size_ = capacity_ = size;
	}
}

QUICKVEC_TEMPLATE
inline void QUICKVEC::resizeWithCushion(size_t size, double growthFactor)
{
	if (size > capacity_)
	{
		size_t new_size = std::round(capacity_ * growthFactor);
		if (new_size < size) { new_size = size; }
		if (new_size < 512) { new_size = 512; }
		else if (new_size < 2048)
		{
			new_size = 2048;
		}
		else if (new_size < 4096)
		{
			new_size = 4096;
		}
		else if (new_size < 8192)
		{
			new_size = 8192;
		}
		reserve(new_size);
	}
	resize(size);
}

QUICKVEC_TEMPLATE
inline void QUICKVEC::resize(size_type size, TT_ val)
{
	size_type old_size = size;
	resize(size);
	if (size > old_size)
	{
		TRACEN("QuickVec", 13, "QUICKVEC::resize initializing %zu elements", size - old_size);
		for (iterator ii = begin() + old_size; ii != end(); ++ii) *ii = val;
	}
}

QUICKVEC_TEMPLATE
inline QUICKVEC_TN::iterator QUICKVEC::insert(const_iterator position, size_t nn, const TT_& val)
{
	assert(position <= end());  // the current end
	size_t offset = position - begin();
	reserve(size_ + nn);  // may reallocate and invalidate "position"

	iterator dst = end() + nn;  // for shifting existing data after
	iterator src = end();       // insertion point
	size_t cnt = end() - (begin() + offset);
	while (cnt--) *--dst = *--src;

	dst = begin() + offset;
	size_ += nn;
	while (nn--) *dst++ = val;
	return begin() + offset;
}

QUICKVEC_TEMPLATE
inline QUICKVEC_TN::iterator QUICKVEC::insert(const_iterator position, const_iterator first, const_iterator last)
{
	assert(position <= end());  // the current end
	size_t nn = (last - first);
	size_t offset = position - begin();
	reserve(size_ + nn);  // may reallocate and invalidate "position"

	iterator dst = end() + nn;  // for shifting existing data after
	iterator src = end();       // insertion point
	size_t cnt = end() - (begin() + offset);
	while (cnt--) *--dst = *--src;

	dst = begin() + offset;
	size_ += nn;
	while (nn--) *dst++ = *first++;
	return begin() + offset;
}

QUICKVEC_TEMPLATE
inline QUICKVEC_TN::iterator QUICKVEC::erase(const_iterator first, const_iterator last)
{
	assert(last <= end());  // the current end
	size_t nn = (last - first);
	size_t offset = first - begin();

	iterator dst = begin() + offset;  // for shifting existing data from last
	iterator src = dst + nn;          // to first
	size_t cnt = end() - src;
	while (cnt--) *dst++ = *src++;

	size_ -= nn;
	return begin() + offset;
}

QUICKVEC_TEMPLATE
inline void QUICKVEC::swap(QuickVec& x) noexcept
{
	TRACEN("QuickVec", 12, "QUICKVEC::swap this=%p enter data_=%p x.data_=%p", (void*)this, (void*)data_, (void*)x.data_);
	std::swap(data_, x.data_);
	std::swap(size_, x.size_);
	std::swap(capacity_, x.capacity_);
	TRACEN("QuickVec", 12, "QUICKVEC::swap return data_=%p x.data_=%p", (void*)data_, (void*)x.data_);
}

QUICKVEC_TEMPLATE
inline void QUICKVEC::push_back(const value_type& val)
{
	if (size_ == capacity_)
	{
		reserve(size_ + size_ / 10 + 1);
	}
	*end() = val;
	++size_;
}

}  // namespace artdaq

#ifdef UNDEF_TRACE_AT_END
#undef TRACEN
#undef UNDEF_TRACE_AT_END
#endif
#endif /* QuickVec_hh */
