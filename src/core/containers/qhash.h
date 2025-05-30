/***********************************************************************
*
* Copyright (c) 2012-2025 Barbara Geller
* Copyright (c) 2012-2025 Ansel Sermersheim
*
* Copyright (c) 2015 The Qt Company Ltd.
* Copyright (c) 2012-2016 Digia Plc and/or its subsidiary(-ies).
* Copyright (c) 2008-2012 Nokia Corporation and/or its subsidiary(-ies).
*
* This file is part of CopperSpice.
*
* CopperSpice is free software. You can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public License
* version 2.1 as published by the Free Software Foundation.
*
* CopperSpice is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*
* https://www.gnu.org/licenses/
*
***********************************************************************/

#ifndef QHASH_H
#define QHASH_H

#include <qcontainerfwd.h>
#include <qhashfunc.h>
#include <qhashfwd.h>
#include <qlist.h>

#include <initializer_list>
#include <unordered_map>

template <typename Key, typename Val, typename Hash = qHashFunc<Key>, typename KeyEqual = qHashEqual<Key>>
class QHashIterator;

template <typename Key, typename Val, typename Hash = qHashFunc<Key>, typename KeyEqual = qHashEqual<Key>>
class QMutableHashIterator;

template <typename Key, typename Val, typename Hash, typename KeyEqual>
class QHash
{
 public:

   class iterator
   {
    public:
      using iterator_category = std::forward_iterator_tag;

      using pointer           = Val *;
      using reference         = Val &;

      using difference_type   = typename std::unordered_map<Key, Val, Hash, KeyEqual>::difference_type;
      using size_type         = typename std::unordered_map<Key, Val, Hash, KeyEqual>::difference_type;
      using value_type        = Val;

      iterator() = default;

      iterator(typename std::unordered_map<Key, Val, Hash, KeyEqual>::iterator iter)
         : m_iter(std::move(iter)) {
      }

      const Key &key() const {
         return m_iter->first;
      }

      Val &value() const {
         return m_iter->second;
      }

      std::pair<const Key, Val> &pair() const {
         return *m_iter;
      }

      // operators
      Val &operator*() const {
         return value();
      }

      Val *operator->() const {
         return &value();
      }

      bool operator==(iterator other) const {
         return m_iter == other.m_iter;
      }

      bool operator!=(iterator other) const {
         return m_iter != other.m_iter;
      }

      iterator &operator+=(size_type n) {
         std::advance(m_iter, n);
         return *this;
      }

      iterator &operator-=(size_type n) {
         std::advance(m_iter, -n);
         return *this;
      }

      iterator operator+(size_type n) const {
         auto tmp = m_iter;
         std::advance(tmp, n);
         return tmp;
      }

      iterator operator-(size_type n) const {
         auto tmp = m_iter;
         std::advance(tmp, -n);
         return tmp;
      }

      iterator &operator++() {
         ++m_iter;
         return *this;
      }

      iterator operator++(int) {
         return m_iter++;
      }

      friend class QHash<Key, Val, Hash, KeyEqual>;

    private:
      typename std::unordered_map<Key, Val, Hash, KeyEqual>::iterator m_iter;
   };

   class const_iterator
   {
    public:
      using iterator_category = std::forward_iterator_tag;

      using pointer           = const Val *;
      using reference         = const Val &;

      using difference_type   = typename std::unordered_map<Key, Val, Hash, KeyEqual>::difference_type;
      using size_type         = typename std::unordered_map<Key, Val, Hash, KeyEqual>::difference_type;
      using value_type        = Val;

      const_iterator() = default;

      const_iterator(typename std::unordered_map<Key, Val, Hash, KeyEqual>::const_iterator iter)
         : m_iter(std::move(iter)) {
      }

      const_iterator(iterator other)
         : m_iter(std::move(other.m_iter)) {
      }

      const Key &key() const {
         return m_iter->first;
      }

      const Val &value() const {
         return m_iter->second;
      }

      const std::pair<const Key, Val> &pair() const {
         return *m_iter;
      }

      // operators
      const Val &operator*() const {
         return value();
      }

      const Val *operator->() const {
         return &value();
      }

      bool operator==(const_iterator other) const {
         return m_iter == other.m_iter;
      }

      bool operator!=(const_iterator other) const {
         return m_iter != other.m_iter;
      }

      const_iterator &operator+=(size_type n) {
         std::advance(m_iter, n);
         return *this;
      }

      const_iterator &operator-=(size_type n) {
         std::advance(m_iter, -n);
         return *this;
      }

      const_iterator operator+(size_type n) const {
         auto tmp = m_iter;
         std::advance(tmp, n);
         return tmp;
      }

      const_iterator operator-(size_type n) const {
         auto tmp = m_iter;
         std::advance(tmp, -n);
         return tmp;
      }

      const_iterator &operator++() {
         ++m_iter;
         return *this;
      }

      const_iterator operator++(int) {
         return m_iter++;
      }

      friend class QHash<Key, Val, Hash, KeyEqual>;

      // free functions
      friend bool operator==(iterator iter1, const_iterator iter2) {
         return iter2.operator==(iter1);
      }

      friend bool operator!=(iterator iter1, const_iterator iter2) {
         return iter2.operator!=(iter1);
      }

    private:
      typename std::unordered_map<Key, Val, Hash, KeyEqual>::const_iterator m_iter;

   };

   using difference_type = typename std::unordered_map<Key, Val, Hash, KeyEqual>::difference_type;
   using pointer         = Val *;
   using reference       = Val &;
   using size_type       = typename std::unordered_map<Key, Val, Hash, KeyEqual>::difference_type;
   using value_type      = Val;

   using key_type        = typename std::unordered_map<Key, Val, Hash, KeyEqual>::key_type;
   using mapped_type     = typename std::unordered_map<Key, Val, Hash, KeyEqual>::mapped_type;
   using hasher          = typename std::unordered_map<Key, Val, Hash, KeyEqual>::hasher;
   using key_equal       = typename std::unordered_map<Key, Val, Hash, KeyEqual>::key_equal;

   using allocator_type  = typename std::unordered_map<Key, Val, Hash, KeyEqual>::allocator_type;

   static constexpr int bucket_count = 1;

   // iterator and const_iterator are classes

   using const_pointer   = const Val *;
   using const_reference = const Val &;

   // java
   using Java_Iterator        = QHashIterator<Key, Val, Hash, KeyEqual>;
   using Java_MutableIterator = QMutableHashIterator<Key, Val, Hash, KeyEqual>;

   QHash() = default;

   QHash(const QHash<Key, Val, Hash, KeyEqual> &other) = default;
   QHash(QHash<Key, Val, Hash, KeyEqual> &&other) = default;

   QHash(std::initializer_list<std::pair<const Key, Val>> list, const Hash &hash = Hash(),
         const KeyEqual &key = KeyEqual())
      : m_data(list, bucket_count, hash, key)
   { }

   explicit QHash(const Hash &hash, const KeyEqual &key = KeyEqual())
      : m_data(hash, key)
   { }

   explicit QHash(const std::unordered_map<Key, Val, Hash, KeyEqual> &other)
      : m_data(other)
   { }

   explicit QHash(std::unordered_map<Key, Val, Hash, KeyEqual> &&other)
      : m_data(std::move(other))
   { }

   template <typename Input_Iterator>
   QHash(Input_Iterator first, Input_Iterator last, const Hash &hash = Hash(), const KeyEqual &key = KeyEqual())
      : m_data(first, last, hash, key)
   { }

   ~QHash() = default;

   // methods
   void clear() {
      m_data.clear();
   }

   size_type capacity() const {
      return m_data.bucket_count();
   }

   bool contains(const Key &key) const {
      auto iter = m_data.find(key);

      if (iter == m_data.end()) {
         return false;
      }

      return true;
   }

   size_type count(const Key &key) const {
      return m_data.count(key);
   }

   size_type count() const {
      return m_data.size();
   }

   bool empty() const {
      return isEmpty();
   }

   bool isEmpty() const {
      return m_data.empty();
   }

   QPair<iterator, iterator> equal_range(const Key &key) {
      return m_data.equal_range(key);
   }

   QPair<const_iterator, const_iterator> equal_range(const Key &key) const {
      return m_data.equal_range(key);
   }

   size_type erase(const Key &key) {
      return m_data.erase(key);
   }

   iterator erase(const_iterator iter) {
      return m_data.erase(iter.m_iter);
   }

   iterator find(const Key &key) {
      // find returns an std::map::iterator, default iterator constructor will convert to QMap::iterator
      return m_data.find(key);
   }

   const_iterator find(const Key &key) const {
      return m_data.find(key);
   }

   const_iterator constFind(const Key &key) const {
      return m_data.find(key);
   }

   iterator insert(const std::pair<const Key, Val> &data) {
      return m_data.insert_or_assign(data.first, data.second).first;
   }

   iterator insert(const Key &key, const Val &value) {
      return m_data.insert_or_assign(key, value).first;
   }

   iterator insert(const Key &key, Val &&value) {
      return m_data.insert_or_assign(key, std::move(value)).first;
   }

   const Key key(const Val &value) const;
   const Key key(const Val &value, const Key &defaultKey) const;

   QList<Key> keys() const;
   QList<Key> keys(const Val &value) const;

   void reserve(size_type size) {
      m_data.reserve(size);
   }

   size_type remove(const Key &key)  {
      return m_data.erase(key);
   }

   size_type size() const {
      // returns unsigned, must convert to signed
      return static_cast<size_type>(m_data.size());
   }

   void squeeze() {
      m_data.rehash(0);
   }

   void swap(QHash<Key, Val, Hash, KeyEqual> &other) {
      qSwap(m_data, other.m_data);
   }

   Val take(const Key &key)  {
      auto iter = m_data.find(key);

      if (iter == m_data.end()) {
         return Val();
      }

      Val retval = std::move(iter->second);
      m_data.erase(iter);

      return retval;
   }

   QHash<Key, Val, Hash, KeyEqual> &unite(const QHash<Key, Val, Hash, KeyEqual> &other) {
      m_data.insert(other.m_data.begin(), other.m_data.end());
      return *this;
   }

   Val value(const Key &key) const {
      auto iter = m_data.find(key);

      if (iter == m_data.end()) {
         // key was not found
         return Val();
      }

      return iter->second;
   }

   Val value(const Key &key, const Val &defaultValue) const {
      auto iter = m_data.find(key);

      if (iter == m_data.end()) {
         // key was not found
         return defaultValue;
      }

      return iter->second;
   }

   QList<Val> values() const;

   // operators
   QHash<Key, Val, Hash, KeyEqual> &operator=(const QHash<Key, Val, Hash, KeyEqual> &other) = default;
   QHash<Key, Val, Hash, KeyEqual> &operator=(QHash<Key, Val, Hash, KeyEqual> &&other) = default;

   bool operator==(const QHash<Key, Val, Hash, KeyEqual> &other) const {
      return m_data == other.m_data;
   }

   bool operator!=(const QHash<Key, Val, Hash, KeyEqual> &other) const {
      return ! (*this == other);
   }

   Val &operator[](const Key &key) {
      return m_data[key];
   }

   const Val operator[](const Key &key) const  {
      return value(key);
   }

   // iterators
   iterator begin() {
      return m_data.begin();
   }

   const_iterator begin() const {
      return m_data.begin();
   }

   const_iterator cbegin() const {
      return m_data.begin();
   }

   const_iterator constBegin() const {
      return m_data.begin();
   }

   iterator end() {
      return m_data.end();
   }

   const_iterator end() const {
      return m_data.end();
   }

   const_iterator cend() const {
      return m_data.end();
   }

   const_iterator constEnd() const {
      return m_data.end();
   }

 private:
   std::unordered_map<Key, Val, Hash, KeyEqual> m_data;
};

// methods

template <typename Key, typename Val, typename Hash, typename KeyEqual>
const Key QHash<Key, Val, Hash, KeyEqual>::key(const Val &value) const
{
   return key(value, Key());
}

template <typename Key, typename Val, typename Hash, typename KeyEqual>
const Key QHash<Key, Val, Hash, KeyEqual>::key(const Val &value, const Key &defaultKey) const
{
   const_iterator iter = begin();

   while (iter != end()) {
      if (iter.value() == value) {
         return iter.key();
      }

      ++iter;
   }

   return defaultKey;
}

template <typename Key, typename Val, typename Hash, typename KeyEqual>
QList<Key> QHash<Key, Val, Hash, KeyEqual>::keys() const
{
   QList<Key> retval;
   const_iterator iter = begin();

   while (iter != end()) {
      retval.append(iter.key());
      ++iter;
   }

   return retval;
}

template <typename Key, typename Val, typename Hash, typename KeyEqual>
QList<Key> QHash<Key, Val, Hash, KeyEqual>::keys(const Val &value) const
{
   QList<Key> retval;
   const_iterator iter = begin();

   while (iter != end()) {
      if (iter.value() == value) {
         retval.append(iter.key());
      }

      ++iter;
   }

   return retval;
}

template <typename Key, typename Val, typename Hash, typename KeyEqual>
QList<Val> QHash<Key, Val, Hash, KeyEqual>::values() const
{
   QList<Val> retval;
   const_iterator iter = begin();

   while (iter != end()) {
      retval.append(iter.value());
      ++iter;
   }

   return retval;
}

// java style iterators

template <typename Key, typename Val, typename Hash, typename KeyEqual>
class QHashIterator
{
   using const_iterator = typename QHash<Key, Val, Hash, KeyEqual>::const_iterator;
   using Item           = const_iterator;

 public:
   QHashIterator(const QHash<Key, Val, Hash, KeyEqual> &hash)
      : c(&hash), i(c->constBegin()), n(c->constEnd())
   {
   }

   ~QHashIterator() = default;

   QHashIterator &operator=(const QHash<Key, Val, Hash, KeyEqual> &hash) {
      c = hash;
      i = c->constBegin();
      n = c->constEnd();

      return *this;
   }

   void toFront() {
      i = c->constBegin();
      n = c->constEnd();
   }

   void toBack() {
      i = c->constEnd();
      n = c->constEnd();
   }

   bool hasNext() const {
      return i != c->constEnd();
   }

   Item next() {
      n = i++;
      return n;
   }

   Item peekNext() const {
      return i;
   }

   bool hasPrevious() const {
      return i != c->constBegin();
   }

   Item previous() {
      n = --i;
      return n;
   }

   Item peekPrevious() const {
      const_iterator p = i;
      return --p;
   }

   const Val &value() const {
      Q_ASSERT(item_exists());
      return *n;
   }

   const Key &key() const {
      Q_ASSERT(item_exists());
      return n.key();
   }

   bool findNext(const Val &value) {
      while ((n = i) != c->constEnd()) {
         if (*i++ == value) {
            return true;
         }
      }

      return false;
   }

   bool findPrevious(const Val &value) {
      while (i != c->constBegin()) {
         if (*(n = --i) == value) {
            return true;
         }
      }

      n = c->constEnd();
      return false;
   }

 private:
   const QHash<Key, Val, Hash, KeyEqual> *c;

   const_iterator i;
   const_iterator n;

   bool item_exists() const {
      return n != c->constEnd();
   }
};

template <typename Key, typename Val, typename Hash, typename KeyEqual>
class QMutableHashIterator
{
   using const_iterator = typename QHash<Key, Val, Hash, KeyEqual>::const_iterator;
   using iterator       = typename QHash<Key, Val, Hash, KeyEqual>::iterator;
   using Item           = iterator;

 public:
   QMutableHashIterator(QHash<Key, Val, Hash, KeyEqual> &hash)
      : c(&hash), i(c->begin()), n(c->end())
   {
   }

   ~QMutableHashIterator() = default;

   QMutableHashIterator &operator=(QHash<Key, Val, Hash, KeyEqual> &hash) {
      c = &hash;
      i = c->begin();
      n = c->end();

      return *this;
   }

   void toFront() {
      i = c->begin();
      n = c->end();
   }

   void toBack() {
      i = c->end();
      n = c->end();
   }

   bool hasNext() const {
      return const_iterator(i) != c->constEnd();
   }

   Item next() {
      n = i++;
      return n;
   }

   Item peekNext() const {
      return i;
   }

   bool hasPrevious() const {
      return const_iterator(i) != c->constBegin();
   }

   Item previous() {
      n = --i;
      return n;
   }

   Item peekPrevious() const {
      iterator p = i;
      return --p;
   }

   void remove() {
      if (const_iterator(n) != c->constEnd()) {
         i = c->erase(n);
         n = c->end();
      }
   }

   void setValue(const Val &value) {
      if (const_iterator(n) != c->constEnd()) {
         *n = value;
      }
   }

   Val &value() {
      Q_ASSERT(item_exists());
      return *n;
   }

   const Val &value() const {
      Q_ASSERT(item_exists());
      return *n;
   }

   const Key &key() const {
      Q_ASSERT(item_exists());
      return n.key();
   }

   bool findNext(const Val &value) {
      while (const_iterator(n = i) != c->constEnd()) {
         if (*i++ == value) {
            return true;
         }
      }

      return false;
   }

   bool findPrevious(const Val &value) {
      while (const_iterator(i) != c->constBegin()) {
         if (*(n = --i) == value) {
            return true;
         }
      }

      n = c->end();
      return false;
   }

 private:
   QHash<Key, Val, Hash, KeyEqual> *c;

   iterator i;
   iterator n;

   bool item_exists() const {
      return const_iterator(n) != c->constEnd();
   }
};

#endif
