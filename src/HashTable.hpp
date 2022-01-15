#pragma once


#include "stdafx.h"
#include <base\util\string_hash.h>
#include <WorldEditor.h>

namespace mh {
	struct node;

	struct node_2
	{
		node_2* lft_;
		node* rht_;
		const char* str_;
	};

	struct node_1
	{
		node_1* next_;
		node* prev_;
		node_2* lft_;
		node* rht_;
		const char* str_;
	};

	struct node
	{
		uint32_t    hash_;
		node_1* next_;
		node* prev_;
		node_2* lft_;
		node* rht_;
		const char*    key;

		bool is_vaild() const
		{
			return ((intptr_t)this > 0x10000);
		}
	};

	template <class Node = node>
	struct entry
	{
		uint32_t step;
		node_1* tail;
		Node* head;

		node* convert(Node* ptr) const
		{
			return (node*)((uintptr_t)ptr + step - 4);
		}
	};

	template <class Node = node>
	struct table
	{
		uint32_t     unk0;
		uint32_t     step;
		uint32_t     tail;
		Node* head;
		uint32_t     unk4;
		uint32_t     unk5;
		uint32_t     unk6;
		entry<Node>* buckets;
		uint32_t     unk8;
		uint32_t     mask;

		class iterator;
		class iterator_v1;

		Node* find(uint32_t hash)
		{
			Node* fnode_ptr = nullptr;

			if (mask == 0xFFFFFFFF)
				return nullptr;

			fnode_ptr = buckets[hash & mask].head;

			if (!fnode_ptr->is_vaild())
				return nullptr;

			for (;;)
			{
				if (fnode_ptr->hash_ == hash)
					return fnode_ptr;
				fnode_ptr = (Node*)(uintptr_t)(fnode_ptr->prev_);

				if (!fnode_ptr->is_vaild())
					return nullptr;
			}
		}

		Node* find(const char* str)
		{
			uint32_t hash;
			Node* fnode_ptr = nullptr;

			if (mask == 0xFFFFFFFF)
				return nullptr;

			hash = base::string_hash(str);

			fnode_ptr = buckets[hash & mask].head;

			if (!fnode_ptr->is_vaild())
				return nullptr;

			for (;;)
			{
				if (fnode_ptr->hash_ == hash)
				{
					if ((const char*)fnode_ptr->key == str)
						return fnode_ptr;

					if (0 == strcmp((const char*)fnode_ptr->key, str))
						return fnode_ptr;
				}
				fnode_ptr = (Node*)(uintptr_t)(fnode_ptr->prev_);

				if (!fnode_ptr->is_vaild())
					return nullptr;
			}
		}


		iterator begin() const
		{
			return iterator(this);
		}

		iterator end() const
		{
			return iterator();
		}
	};

	template <class Node>
	class table<Node>::iterator
	{
	public:
		typedef Node value_type;
		typedef value_type& reference;
		typedef value_type* pointer;

	public:
		iterator()
			: ptr_(nullptr)
			, current_(nullptr)
		{ }

		explicit iterator(const table<Node>* ptr)
			: ptr_(ptr)
			, current_(ptr->head)
		{
			if (!current_->is_vaild()) {
				current_ = nullptr;
			}
		}

		~iterator()
		{ }

		reference operator*() const
		{
			return *current_;
		}

		pointer operator->() const
		{
			return current_;
		}

		iterator operator++(int)
		{
			auto result = *this;
			++(*this);
			return result;
		}

		iterator& operator++()
		{
			current_ = (Node*)((uint32_t*)current_)[ptr_->step / 4 + 1];
			if (!current_->is_vaild()) {
				current_ = nullptr;
			}
			return *this;
		}

		bool operator==(const iterator& other) const
		{
			return current_ == other.current_;
		}

		bool operator!=(const iterator& other) const
		{
			return !operator==(other);
		}

	private:
		const table<Node>* ptr_;
		Node* current_;
	};

	template <class Node>
	class table<Node>::iterator_v1
	{
	public:
		typedef Node value_type;
		typedef value_type& reference;
		typedef value_type* pointer;

	public:
		iterator_v1()
			: ptr_(nullptr)
			, index_(0)
			, current_(nullptr)
		{ }

		explicit iterator_v1(const table<Node>* ptr)
			: ptr_(ptr)
			, index_(0)
			, current_(nullptr)
		{
			if (!ptr_->buckets)
			{
				ptr_ = nullptr;
				return;
			}
			operator++();
		}

		~iterator_v1()
		{ }

		reference operator*() const
		{
			return *current_;
		}

		pointer operator->() const
		{
			return current_;
		}

		iterator_v1 operator++(int)
		{
			auto result = *this;
			++(*this);
			return result;
		}

		iterator_v1& operator++()
		{
			if (!current_)
			{
				if (index_ > ptr_->mask)
				{
					return *this;
				}

				current_ = ptr_->buckets[index_].head;
			}
			else
			{
				current_ = (Node*)(uintptr_t)(current_->prev_);
			}

			if (!current_->is_vaild())
			{
				index_++;
				current_ = nullptr;
				return operator++();
			}
			return *this;
		}

		bool operator==(const iterator_v1& other) const
		{
			return current_ == other.current_;
		}

		bool operator!=(const iterator_v1& other) const
		{
			return !operator==(other);
		}

		//private:
		const table<Node>* ptr_;
		uint32_t     index_;
		Node* current_;
	};


	struct config_subnode_parent_head
	{
		uint32_t unknow;
	};

	struct config_subnode_parent : public config_subnode_parent_head, public node
	{
		
	};

	struct config_subnode : public node
	{
		uint32_t data;
		config_subnode_parent* parent;
	};

	struct config_table_node : public node
	{
		typedef table<config_subnode> table_t;
		typedef table_t::iterator iterator;
		table_t subtable;

		iterator begin()
		{
			return iterator(&subtable);
		}

		iterator end()
		{
			return iterator();
		}
	};

	struct config_table
	{

		typedef table<config_table_node> table_t;
		typedef table_t::iterator iterator;

		uint32_t vtable;
		uint32_t unknow4;
		table_t table_;

		iterator begin()
		{
			return iterator(&table_);
		}

		iterator end()
		{
			return iterator();
		}

		config_table_node* find(const char* str)
		{
			return table_.find(str);
		}
	};


	
	inline config_table* get_config_table() {
		return *(mh::config_table**)(WorldEditor::getAddress(0x820004));
	}


}