#ifndef ARRAY_H
#define ARRAY_H

typedef long unsigned int LUint;
#include <cassert>

template <class T>
class Array
{
	public:
		Array() : start(NULL), size(0), reserved(0) {}
		Array(LUint newSize) : start(new T[newSize]), size(newSize), reserved(newSize) {}
		~Array() {delete [] start;}

		void Empty() {size = 0;}
		void Add(const T & add);
		void Reserve(LUint reserve);
		void Resize(LUint newSize);
		void RemoveBack();

		LUint Size() const {return size;}
		LUint Capacity() const {return reserved;}

		void operator=(const Array & equ);
		bool operator==(const Array & equ) const;
		bool operator!=(const Array & equ) const {return !operator==(equ);}

		class Iterator
		{
			public:
				Iterator(const Array & from) : parent(from), index(0) {}
				Iterator(const Iterator & cpy) : parent(cpy.parent), index(cpy.index) {}
				~Iterator() {}

				bool Good() const {return index < parent.Size();}

				T & operator*() const {return parent.start[index];}

				void operator++() {++index;}
				void operator--() {--index;}
				void operator++(int) {operator++();}
				void operator--(int) {operator--();}
				Iterator & operator+=(int amount) {index += amount;}
				Iterator & operator-=(int amount) {index -= amount;}
				Iterator operator+(int amount) {return Iterator(*this) += amount;}
				Iterator operator-(int amount) {return Iterator(*this) -= amount;}

				void operator=(const Iterator & set) {index = set.index;}
				bool operator==(const Iterator & set) {return (&parent == &(set.parent) && index == set.index);}
			private:
				const Array & parent;
				LUint index;
		};

		Iterator First() const {return Iterator(*this);}
		Iterator Last() const {return Iterator(*this) -= (size-1);}
		

		T & operator[](LUint at) const 
		{
			#ifdef DEBUGALL
				printf("	Array<T>::operator[] - called for index %lu/%lu (reserved %lu)\n", at, size, reserved);
				
			#endif //DEBUG
			assert(at < size); return start[at];
		}
		
		int Find(const T & find)
		{

			LUint result;
			for (result = 0; result < size; result++)
			{	
				//printf("%p %lu/%lu\n", (void*)(start), result, size);
				if (start[result] == find)
					return (int)(result);
			}		
			return -1;
		}

	private:
		T * start;
		LUint size; LUint reserved;
};

template <class T> void Array<T>::Add(const T & add)
{
	if (size >= reserved)
	{
		T * old = start; 
		reserved *= 2; ++reserved;
		start = new T[reserved];
		for (LUint ii=0; ii < size; ++ii)
			start[ii] = old[ii];
		delete [] old;
	}
	start[size++] = add;
}

template <class T> void Array<T>::RemoveBack()
{
	if (size > 0)
		--size;
}

template <class T> void Array<T>::Resize(LUint newSize)
{
	T * old = start;
	start = new T[newSize];
	for (LUint ii=0; ii < size; ++ii)
		start[ii] = old[ii];
	size = newSize; reserved = newSize;
	delete [] old;
}

template <class T> void Array<T>::Reserve(LUint newReserve)
{
	if (newReserve > reserved)
	{
		T * old = start;
		start = new T[newReserve];
		for (LUint ii=0; ii < size; ++ii)
			start[ii] = old[ii];
		reserved = newReserve;
	}
}

#endif //ARRAY_H
