#ifndef TPL_VECTOR_H
#define TPL_VECTOR_H

#ifndef ITERATE
#define ITERATE(collection,enumerator) for(uint32 enumerator = 0; enumerator < (collection).get_count(); enumerator++)
#endif

#ifndef ITERATE_PTR
#define ITERATE_PTR(collection,enumerator) for(uint32 enumerator = 0; enumerator < (collection)->get_count(); enumerator++)
#endif 

#include "../macros.h"
#include "../simtypes.h"
#include "../simdebug.h"

#ifdef _MSC_VER
#include <typeinfo.h>
#else
#include <typeinfo>
#endif

template<class T> class vector_tpl;
template<class T> inline void swap(vector_tpl<T>& a, vector_tpl<T>& b);


/** A template class for a simple vector type */
template<class T> class vector_tpl
{
	public:
		typedef const T* const_iterator;
		typedef       T* iterator;

		/** Construct a vector for cap elements */
		vector_tpl() : data(NULL), size(0), count(0) {}

		explicit vector_tpl(const uint32 cap) :
			data(cap > 0 ? new T[cap] : NULL),
			size(cap),
			count(0) {}

		vector_tpl(const vector_tpl& copy_from) :
			data( copy_from.get_size() > 0 ? new T[ copy_from.get_size() ] : 0 ),
			size( copy_from.get_size() ),
			count( copy_from.get_count() ) {
				for( uint32 i = 0; i < count; i++ ) {
					data[i] = copy_from.data[i];
				}
			}

		vector_tpl& operator=( vector_tpl const& other ) { 
			vector_tpl tmp(other); 
			swap(tmp, *this); 
			return *this;
		}

		~vector_tpl() { delete [] data; }

		/** sets the vector to empty */
		void clear() { count = 0; }

		/**
		 * Resizes the maximum data that can be hold by this vector.
		 * Existing entries are preserved, new_size must be big enough to hold them
		 */
		void resize(const uint32 new_size)
		{
			if (new_size <= size) return; // do nothing

			T* new_data = new T[new_size];
			if(size>0) {
				for (uint32 i = 0; i < count; i++) {
					new_data[i] = data[i];
				}
				delete [] data;
			}
			size = new_size;
			data = new_data;
		}

		/**
		 * Checks if element elem is contained in vector.
		 * Uses the == operator for comparison.
		 */
		bool is_contained(const T& elem) const
		{
			for (uint32 i = 0; i < count; i++) {
				if (data[i] == elem) {
					return true;
				}
			}
			return false;
		}

		/**
		 * Checks if element elem is contained in vector.
		 * Uses the == operator for comparison.
		 */
		uint32 index_of(const T& elem) const
		{
			for (uint32 i = 0; i < count; i++) {
				if (data[i] == elem) {
					return i;
				}
			}
			//assert(false);
			return 0xFFFFFFFFu;
		}

		void append(const T& elem)
		{
			if(  count == size  ) {
				resize(size == 0 ? 1 : size * 2);
			}
			data[count++] = elem;
		}

		/**
		 * Checks if element is contained. Appends only new elements.
		 * extend vector if nessesary
		 */
		bool append_unique(const T& elem)
		{
			if (is_contained(elem)) {
				return false;
			}
			append(elem);
			return true;
		}

		/** insert data at a certain pos */
		void insert_at(const uint32 pos, const T& elem)
		{
			if (pos < count) {
				if (count == size) {
					resize(size == 0 ? 1 : size * 2);
				}
				for (uint i = count; i > pos; i--) {
					data[i] = data[i - 1];
				}
				data[pos] = elem;
				count++;
			}
			else {
				append(elem);
			}
		}

		/**
		 * Insert `elem' with respect to ordering.
		 */
		template<class StrictWeakOrdering>
		void insert_ordered(const T& elem, StrictWeakOrdering comp)
		{
			sint32 low = -1, high = count;
			while(  high - low>1  ) {
				const sint32 mid = ((uint32) (low + high)) >> 1;
				T &mid_elem = data[mid];
				if(  comp(elem, mid_elem)  ) {
					high = mid;
				}
				else {
					low = mid;
				}
			}
			insert_at(high, elem);
		}

		/**
		 * Only insert `elem' if not already contained in this vector.
		 * Respects the ordering and assumes the vector is ordered.
		 * Returns NULL if insertion is successful;
		 * otherwise return the address of the element in conflict
		 */
		template<class StrictWeakOrdering>
		T* insert_unique_ordered(const T& elem, StrictWeakOrdering comp)
		{
			sint32 low = -1, high = count;
			while(  high - low>1  ) {
				const sint32 mid = ((uint32) (low + high)) >> 1;
				T &mid_elem = data[mid];
				if(  elem==mid_elem  ) {
					return &mid_elem;
				}
				else if(  comp(elem, mid_elem)  ) {
					high = mid;
				}
				else {
					low = mid;
				}
			}
			insert_at(high, elem);
			return NULL;
		}

		/**
		 * put the data at a certain position
		 * BEWARE: using this function will create default objects, depending on
		 * the type of the vector
		 */
		void store_at(const uint32 pos, const T& elem)
		{
			if (pos >= size) {
				resize((pos & 0xFFFFFFF7) + 8);
			}
			data[pos] = elem;
			if (pos >= count) {
				count = pos + 1;
			}
		}

		/** removes element, if contained */
		void remove(const T& elem)
		{
			for (uint32 i = 0; i < count; i++) {
				if (data[i] == elem) {
					return remove_at(i);
				}
			}
		}

		/** removes element at position */
		void remove_at(const uint32 pos, const bool preserve_order = true)
		{
			assert(pos<count);
			count--;
			if(preserve_order)
			{
				for (uint i = pos; i < count; i++) {
					data[i] = data[i + 1];
				}
			}
			else if(  pos!=(count)  )
				{
				data[pos] = data[count];
			}
		}

		T& get_element(uint e)
		{
			return (*this)[e];
		}

		T const& get_element(uint e) const
		{
			return (*this)[e];
		}
		
		T& operator [](uint i)
		{
			if (i >= count) {
				dbg->fatal("vector_tpl<T>::[]", "%s: index out of bounds: %i not in 0..%d", typeid(T).name(), i, count - 1);
			}
			return data[i];
		}

		const T& operator [](uint i) const
		{
			if (i >= count) {
				dbg->fatal("vector_tpl<T>::[]", "%s: index out of bounds: %i not in 0..%d", typeid(T).name(), i, count - 1);
			}
			return data[i];
		}

		T& front() const { return data[0]; }

		T& back() const { return data[count - 1]; }

		iterator begin() { return data; }
		iterator end()   { return data + count; }

		const_iterator begin() const { return data; }
		const_iterator end()   const { return data + count; }

		/** Get the number of elements in the vector */
		uint32 get_count() const { return count; }

		/** Get the capacity */
		uint32 get_size() const { return size; }

		bool empty() const { return count == 0; }

	private:
		T* data;
		uint32 size;  ///< Capacity
		uint32 count; ///< Number of elements in vector


	friend void swap<>(vector_tpl<T>& a, vector_tpl<T>& b);
};


template<class T> void swap(vector_tpl<T>& a, vector_tpl<T>& b)
{
	sim::swap(a.data,  b.data);
	sim::swap(a.size,  b.size);
	sim::swap(a.count, b.count);
}

/**
 * Clears vectors of the type vector_tpl<someclass*>
 * Deletes all objects pointed to by pointers in the vector
 */
template<class T> void clear_ptr_vector(vector_tpl<T*>& v)
{
	for(uint32 i=0; i<v.get_count(); i++) {
		delete v[i];
	}
	v.clear();
}



#endif
