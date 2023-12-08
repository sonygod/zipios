#pragma once
#ifndef CATCH_TESTS_HPP
#define CATCH_TESTS_HPP

/*
  Zipios -- a small C++ library that provides easy access to .zip files.

  Copyright (C) 2000-2007  Thomas Sondergaard
  Copyright (c) 2015-2022  Made to Order Software Corp.  All Rights Reserved

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

/** \file
 * \brief Common header for all our catch tests.
 *
 * Zipios comes with a unit test suite. This header defines things
 * that all the tests access, such as the catch.hpp header file.
 */

#include <zipios/zipios-config.hpp>

#include <catch2/snapcatch2.hpp>

#include <memory>
#include <sstream>

#include <limits.h>


#if defined(__sun) || defined(__sun__) || defined(__SunOS) || defined(__CYGWIN__)
namespace std
{

// somehow they have g++ 4.8.2 but to_string() is missing
template<typename T>
std::string to_string(T v)
{
    std::ostringstream ss;
    ss << v;
    return ss.str();
}

}
#endif


namespace zipios_test
{



/** \brief Create a random number representing a size_t object.
 *
 * This function is used to accommodate 32 and 64 bit tests. It creates
 * a random number in a size_t variable. The result is expected to fill
 * all the bits in a random manner.
 *
 * \todo
 * Look into whether we want to define all the bits. Right now, it is
 * likely that bit 31 and 63 never get set.
 *
 * \return A random size_t variable.
 */
inline size_t rand_size_t()
{
    return static_cast<size_t>(rand())
#if !defined(_ILP32) && ((UINT_MAX) != 0xFFFFFFFFU)
         | (static_cast<size_t>(rand()) << 32)
#endif
        ;
}


class auto_unlink_t
{
public:
                            auto_unlink_t(std::string const & filename, bool delete_on_creation);
                            ~auto_unlink_t();

    void                    unlink();

private:
    std::string const       m_filename;
};


// original found in snapdev
class safe_chdir
{
public:
                                safe_chdir(std::string const & path);
                                safe_chdir(safe_chdir const & rhs) = delete;
                                ~safe_chdir();

    safe_chdir &                operator = (safe_chdir const & rhs) = delete;

private:
    std::unique_ptr<char>       m_original_path = std::unique_ptr<char>();
};


class file_t
{
public:
    typedef std::shared_ptr<file_t>     pointer_t;
    typedef std::vector<pointer_t>      vector_t;
    typedef std::vector<std::string>    filenames_t;

    enum class type_t
    {
        UNKNOWN,
        REGULAR,
        DIRECTORY
    };

    file_t(type_t t, int children_count, std::string const & new_filename = "");
    ~file_t();
    type_t type() const;
    std::string const & filename() const;
    vector_t const & children() const;
    size_t size();
    type_t find(std::string const & name);
    filenames_t get_all_filenames() const;

private:
    void get_filenames(filenames_t & names, std::string const & parent) const;

    std::string     m_filename;
    vector_t        m_children;
    type_t          m_type;
};


} // zipios_test namespace

// Local Variables:
// mode: cpp
// indent-tabs-mode: nil
// c-basic-offset: 4
// tab-width: 4
// End:

// vim: ts=4 sw=4 et
#endif
