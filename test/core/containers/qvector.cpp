/***********************************************************************
*
* Copyright (c) 2012-2025 Barbara Geller
* Copyright (c) 2012-2025 Ansel Sermersheim
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

#include <qvector.h>

#include <cs_catch2.h>

TEST_CASE("QVector traits", "[qvector]")
{
   REQUIRE(std::is_copy_constructible_v<QVector<int>> == true);
   REQUIRE(std::is_move_constructible_v<QVector<int>> == true);

   REQUIRE(std::is_copy_assignable_v<QVector<int>> == true);
   REQUIRE(std::is_move_assignable_v<QVector<int>> == true);

   REQUIRE(std::has_virtual_destructor_v<QVector<int>> == false);
}

TEST_CASE("QVector append", "[qvector]")
{
   QVector<QString> v = { "watermelon", "apple", "pear", "grapefruit" };

   v.append("quince");

   REQUIRE(v.contains("quince"));
   REQUIRE(v[4] == "quince");
   REQUIRE(v.length() == 5);
}

TEST_CASE("QVector clear", "[qvector]")
{
   QVector<QString> v = { "watermelon", "apple", "pear", "grapefruit" };

   v.clear();

   REQUIRE(v.size() == 0);
}

TEST_CASE("QVector contains_a", "[qvector]")
{
   QVector<QString> v = { "watermelon", "apple", "pear", "grapefruit" };

   REQUIRE(v.contains("pear"));
   REQUIRE(! v.contains("orange"));
}

TEST_CASE("QVector contains_b", "[qvector]")
{
   QVector<unsigned int> v = { 5, 11, 23 };

   unsigned int data1 = 5;
   int data2 = 5;

   REQUIRE(v.contains(data1) == true);
   REQUIRE(v.contains(data2) == true);
}

TEST_CASE("QVector empty", "[qvector]")
{
   QVector<QString> v;

   REQUIRE(v.isEmpty());
}

TEST_CASE("QVector equality", "[qvector]")
{
   QVector<QString> v1 = { "watermelon", "apple", "pear", "grapefruit" };
   QVector<QString> v2 = { "watermelon", "apple", "pear", "grapefruit" };

   REQUIRE(v1 == v2);
   REQUIRE(! (v1 != v2));
}

TEST_CASE("QVector erase", "[qvector]")
{
   QVector<QString> v = { "watermelon", "apple", "pear", "grapefruit" };

   v.erase(v.begin() + 1);

   REQUIRE(! v.contains("apple"));

   REQUIRE(v.contains("watermelon"));
   REQUIRE(v.contains("pear"));
   REQUIRE(v.contains("grapefruit"));

   REQUIRE(v.length() == 3);
}

TEST_CASE("QVector indexOf_a", "[qvector]")
{
   QVector<QString> v = { "watermelon", "apple", "pear", "grapefruit" };

   REQUIRE(v.indexOf("apple") == 1);
   REQUIRE(v.indexOf("berry") == -1);
}

TEST_CASE("QVector indexOf_b", "[qvector]")
{
   QVector<unsigned int> v = { 5, 11, 23 };

   unsigned int data1 = 5;
   int data2 = 5;

   REQUIRE(v.indexOf(data1) == 0);
   REQUIRE(v.indexOf(data2) == 0);
}

TEST_CASE("QVector insert", "[qvector]")
{
   QVector<QString> v = { "watermelon", "apple", "pear", "grapefruit" };

#if ! defined(Q_CC_MSVC)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
#endif

   v.insert(1, "mango");

#if ! defined(Q_CC_MSVC)
#pragma GCC diagnostic pop
#endif

   REQUIRE(v.contains("mango"));
   REQUIRE(v[1] == "mango");
   REQUIRE(v.length() == 5);
}

TEST_CASE("QVector length", "[qvector]")
{
   QVector<QString> v = { "watermelon", "apple", "pear", "grapefruit" };

   REQUIRE(v.length() == 4);
   REQUIRE(v.size() == 4);
}

TEST_CASE("QVector position", "[qvector]")
{
   QVector<QString> v = { "watermelon", "apple", "pear", "grapefruit" };

   REQUIRE(v.first() == "watermelon");
   REQUIRE(v.last()  == "grapefruit");

   REQUIRE(v.front() == "watermelon");
   REQUIRE(v.back()  == "grapefruit");
}

TEST_CASE("QVector prepend", "[qvector]")
{
   QVector<QString> v = { "watermelon", "apple", "pear", "grapefruit" };

   v.prepend("quince");

   REQUIRE(v.contains("quince"));
   REQUIRE(v[0] == "quince");
   REQUIRE(v.length() == 5);
}

TEST_CASE("QVector remove", "[qvector]")
{
   QVector<QString> v = { "watermelon", "apple", "pear", "grapefruit" };

   v.removeOne("apple");
   v.remove(0);

   REQUIRE(! v.contains("apple"));
   REQUIRE(! v.contains("watermelon"));

   REQUIRE(v.contains("pear"));
   REQUIRE(v.contains("grapefruit"));

   REQUIRE(v.length() == 2);
}