#pragma once
#include "cIGZAllocatorService.h"
#include "GZServPtrs.h"
#include <functional>

template<typename T>
struct SC4ListNode
{
	SC4ListNode<T>* next;
	SC4ListNode<T>* previous;
	T* value;
};

template <typename T>
struct SC4ListIterator
{
	using iterator_category = std::forward_iterator_tag;
	using difference_type = std::ptrdiff_t;
	using value_type = T;
	using pointer = value_type*;
	using reference = value_type&;

	SC4ListIterator(SC4ListNode<T>* node) : pNode(node)
	{
	}

	~SC4ListIterator()
	{
	}

	reference operator*() const
	{
		return *pNode->value;
	}

	pointer operator->()
	{
		return pNode->value;
	}

	// Prefix increment
	SC4ListIterator& operator++()
	{
		pNode = pNode->next;
		return *this;
	}

	// Postfix increment
	SC4ListIterator operator++(int)
	{
		SC4ListIterator tmp = *this;
		++(*this);
		return tmp;
	}

	friend bool operator== (const SC4ListIterator& a, const SC4ListIterator& b)
	{
		return a.pNode == b.pNode;
	};
	friend bool operator!= (const SC4ListIterator& a, const SC4ListIterator& b)
	{
		return a.pNode != b.pNode;
	};

private:
	SC4ListNode<T>* pNode;
};

template <typename T>
class SC4List
{
public:
	typedef SC4ListIterator<const T> const_iterator;
	typedef SC4ListIterator<T> iterator;

	SC4List()
	{
		root.next = &root;
		root.previous = &root;
	}

	~SC4List()
	{
		cIGZAllocatorServicePtr allocator;

		auto pRoot = &root;
		auto entry = root.next;

		while (entry != pRoot)
		{
			auto nextEntry = entry->next;

			T* pValue = entry->value;

			if (pValue)
			{
				if (std::is_base_of<cIGZUnknown, T>::value)
				{
					pValue->Release();
				}
				else
				{
					pValue->~T();
				}
			}

			allocator->Deallocate(entry);

			entry = nextEntry;
		}

		root.next = pRoot;
		root.previous = pRoot;
	}

	iterator begin()
	{
		return iterator(root.next);
	}

	const_iterator cbegin()
	{
		return const_iterator(root.next);
	}

	iterator end()
	{
		return iterator(&root);
	}

	const_iterator cend()
	{
		return const_iterator(&root);
	}

	bool empty() const
	{
		return root.next == &root;
	}

	size_t size() const
	{
		size_t size = 0;

		auto pRoot = &root;
		auto entry = root.next;

		while (entry != pRoot)
		{
			auto nextEntry = entry->next;

			++size;

			entry = nextEntry;
		}

		return size;
	}

private:
	SC4ListNode<T> root;
};
