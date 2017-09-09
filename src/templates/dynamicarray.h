/*
	*** Dynamic Array
	*** src/templates/dynamicarray.h
	Copyright T. Youngs 2012-2017

	This file is part of dUQ.

	dUQ is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	dUQ is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with dUQ.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef DUQ_DYNAMICARRAY_H
#define DUQ_DYNAMICARRAY_H

#include "templates/list.h"
#include "base/messenger.h"
#include <new>

using namespace std;

// Array Chunk
template <class T> class ArrayChunk
{
	/*
	 * Chunk of objects, maintained by a DynamicArray
	 */
	public:
	// Constructor
	ArrayChunk<T>()
	{
		objectSize_ = 0;
		objectArray_ = NULL;
		objectUsed_ = NULL;
		nextAvailableObject_ = -1;
		nUnusedObjects_ = 0;
		
		prev = NULL;
		next = NULL;
	}
	// Destructor
	~ArrayChunk()
	{
		if (objectArray_) delete[] objectArray_;
		if (objectUsed_) delete[] objectUsed_;
	}
	// List pointers
	ArrayChunk<T>* prev, *next;


	/*
	 * Chunk Data
	 */
	private:
	// Size of individual object
	int objectSize_;
	// Object array
	T* objectArray_;
	// Object usage flags
	bool* objectUsed_;
	// Number of unused objects in chunk
	int nUnusedObjects_;
	// Index of next available object
	int nextAvailableObject_;

	private:
	// Determine array offset of object
	int objectOffset(T* object)
	{
	// 	printf("in objectoffset: %li %li\n", intptr_t(object), intptr_t(&objectArray_[0]));
		intptr_t offset = intptr_t(object) - intptr_t(&objectArray_[0]);
	// 	printf("Offset = %li\n", offset);
		if (offset < 0) return -1;
		int index = offset / objectSize_;
		return (index < FACTORYCHUNKSIZE ? index : -1);
	}

	public:
	// Initialise chunk
	bool initialise()
	{
		// Allocate object array
		try
		{
			objectArray_ = new T[FACTORYCHUNKSIZE];
		}
		catch (bad_alloc& alloc)
		{
			Messenger::error("ArrayChunk<T>() - Failed to allocate sufficient memory for objectArray_. Exception was : %s\n", alloc.what());
			return false;
		}

		// Allocate object used flags
		try
		{
			objectUsed_ = new bool[FACTORYCHUNKSIZE];
		}
		catch (bad_alloc& alloc)
		{
			Messenger::error("ArrayChunk<T>() - Failed to allocate sufficient memory for objectArray_. Exception was : %s\n", alloc.what());
			return false;
		}

		objectSize_ = sizeof(T);
		for (int n=0; n<FACTORYCHUNKSIZE; ++n) objectUsed_[n] = false;
		nextAvailableObject_ = 0;
		nUnusedObjects_ = FACTORYCHUNKSIZE;
	}
	// Return next available object
	T* nextAvailable()
	{
		if (nextAvailableObject_ == -1) return NULL;
		T* object = &objectArray_[nextAvailableObject_];
		objectUsed_[nextAvailableObject_] = true;
		--nUnusedObjects_;

		// If there are no more unused objects, then we are done
		if (nUnusedObjects_ == 0)
		{
			nextAvailableObject_ = -1;
			return object;
		}

		// Search for next available object before we return the object
		int nextFree = nextAvailableObject_ + 1;
		// -- First part - search to end of current array
		while (nextFree < FACTORYCHUNKSIZE)
		{
			if (!objectUsed_[nextFree])
			{
				nextAvailableObject_ = nextFree;
				return object;
			}
			++nextFree;
		}
		// -- Second part - search beginning of array up to current position
		nextFree = 0;
		while (nextFree < nextAvailableObject_)
		{
			if (!objectUsed_[nextFree])
			{
				nextAvailableObject_ = nextFree;
				return object;
			}
			++nextFree;
		}

		// Shouldn't get here!
		printf("Internal Error - ArrayChunk.\n");
		return object;
	}
	// Return specified object to pool
	bool returnObject(T* object)
	{
		// Get the item offset of the object
		int offset = objectOffset(object);
		if (offset == -1) return false;
		
		// Mark the object as unused, and increase the unused counter
		objectUsed_[offset] = false;

		++nUnusedObjects_;
		if (nextAvailableObject_ == -1) nextAvailableObject_ = offset;
		return true;
	}
	// Return whether object is part of this chunk
	bool ownsObject(T* object)
	{
		// Calculate array offset of this object
		return (objectOffset(object) != -1);
	}
	// Return whether there are unused objects in the chunk
	bool hasUnusedObjects()
	{
		return (nUnusedObjects_ != 0);
	}
};

// Dynamic Array Class
template <class T> class DynamicArray
{
	public:
	// Constructor
	DynamicArray<T>()
	{
		currentChunk_ = NULL;
	}


	/*
	 * Storage
	 */
	private:
	// List of object chunks maintained by this factory
	List< ArrayChunk<T> > arrayChunks_;
	// Current chunk from which objects are being taken
	ArrayChunk<T>* currentChunk_;

	private:
	// Produce a new object
	T* produce()
	{
		if (currentChunk_ == NULL)
		{
			currentChunk_ = arrayChunks_.add();
			if (!currentChunk_->initialise()) return NULL;
			return currentChunk_->nextAvailable();
		}
		else if (currentChunk_->hasUnusedObjects()) return currentChunk_->nextAvailable();
		else
		{
			// Must search current chunk list to see if any current chunks have available space. If not, we will create a new one
			for (ArrayChunk<T>* chunk = arrayChunks_.first(); chunk != NULL; chunk = chunk->next)
			{
				if (chunk == currentChunk_) continue;
				if (chunk->hasUnusedObjects())
				{
					currentChunk_ = chunk;
					return currentChunk_->nextAvailable();
				}
			}

			// No dice - make a new chunk
			currentChunk_ = arrayChunks_.add();
			if (!currentChunk_->initialise()) return NULL;
			return currentChunk_->nextAvailable();
		}

		// If we get here, then something has gone horribly wrong...
		printf("Internal Error - Couldn't find an empty chunk to return an object from.\n");
		return NULL;
	}
	// Return specified object to chunk stack
	void returnObject(T* object)
	{
		// Must find chunk which owns this object
		for (ArrayChunk<T>* chunk = arrayChunks_.first(); chunk != NULL; chunk = chunk->next)
		{
			if (chunk->returnObject(object))
			{
				object->setArrayIndex(-1);
				return;
			}
		}

		// Couldn't find it!
		printf("Internal Error - Tried to return an object to an ObjectFactory which didn't produce it.\n");
	}


	/*
	 * Array Creation / Initialisation
	 */
	private:
	// Array of pointers to our objects
	Array<T*> array_;

	public:
	// Clear array, returning all objects to the pool
	void clear()
	{
		for (int n = 0; n<array_.nItems(); ++n) returnObject(array_[n]);
		array_.clear();
	}
	// Initialise array to specified size, creating objects from factory
	bool initialise(int nItems)
	{
		array_.initialise(nItems);
	}


	/*
	 * Set / Get
	 */
	public:
	// Add single item to end of array
	T* add()
	{
		T* newItem = produce();
		newItem->setArrayIndex(array_.nItems());
		array_.add(newItem);
	}
	// Add multiple items to end of array
	void add(int count)
	{
		T* newItem;
		for (int n=0; n<count; ++n)
		{
			newItem = produce();
			newItem->setArrayIndex(array_.nItems());
			array_.add(newItem);
		}
	}
	// Remove item at specified index and reorder so we still have a contiguous array
	void removeWithReorder(int index)
	{
		// Return the object at the specified index
		returnObject(array_[index]);

		// Now, we will simply swap the last item in the array for this one, changing its index in the process (unless it is the last item in the list)
		if (index != (array_.nItems()-1))
		{
			array_[index] = array_[array_.nItems()-1];
			array_[index]->setArrayIndex(index);
		}

		// Tell the array to drop the last item in the list
		array_.removeLast();
	}
	// Return size of item array
	int nItems() const
	{
		return array_.nItems();
	}
	// Return item array
	T** array()
	{
		return array_.array();
	}
	// Element access operator
	T* operator[](int index)
	{
#ifdef CHECKS
		if ((index < 0) || (index >= array_.nItems()))
		{
			Messenger::error("Array index %i is out of bounds (array size = %i).\n", index, array_.nItems());
			return NULL;
		}
#endif
		return array_[index];
	}
};

#endif
