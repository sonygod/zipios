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
 *
 * Zipios unit tests used to verify the VirtualSeeker class.
 */

#include "catch_main.hpp"

#include <zipios/virtualseeker.hpp>
#include <zipios/zipiosexceptions.hpp>

#include <fstream>

#include <unistd.h>


namespace
{


size_t const FOUR(4);


} // no name namespace


CATCH_TEST_CASE("VirtualSeeker tests", "[zipios_common]")
{
    zipios_test::safe_chdir cwd(SNAP_CATCH2_NAMESPACE::g_tmp_dir());

    // create a file of 256 bytes
    zipios_test::auto_unlink_t auto_unlink("file256.bin", true);
    {
        std::ofstream os("file256.bin", std::ios::out | std::ios::binary);
        for(int i(0); i < 256; ++i)
        {
            os << static_cast<char>(i);
        }
    }

    // reopen as read-only
    std::ifstream is("file256.bin", std::ios::in | std::ios::binary);
    char buf[256];

    for(int count(0); count < 256; ++count)
    {
        // make the start betwee 0 and 200 so that we have some wiggle room
        // for the end offset
        //
        zipios::offset_t const start_offset(rand() % 200);
        zipios::offset_t const end_offset(start_offset + rand() % (256 - start_offset));
        CATCH_REQUIRE(start_offset <= end_offset); // this should always be true
        zipios::offset_t const end(256 - end_offset);
        size_t const max_read(end_offset - start_offset);
        // note that the "gap" may be zero

        // attempt to create the seeker with invalid offsets
        CATCH_REQUIRE_THROWS_AS([&](){
                        zipios::VirtualSeeker vs(start_offset, -end);
                    }(), zipios::InvalidException);
        CATCH_REQUIRE_THROWS_AS([&](){
                        zipios::VirtualSeeker vs(-start_offset, -end);
                    }(), zipios::InvalidException);
        if(start_offset != 0)
        {
            CATCH_REQUIRE_THROWS_AS([&](){
                            zipios::VirtualSeeker vs(-start_offset, end);
                        }(), zipios::InvalidException);
        }

        // the end parameter to the VirtualSeeker is a "weird" position
        zipios::VirtualSeeker vs(start_offset, end);

        {
            CATCH_REQUIRE(vs.startOffset() == start_offset);
            CATCH_REQUIRE(vs.endOffset() == end);

            zipios::offset_t start_test;
            zipios::offset_t end_test;
            vs.getOffsets(start_test, end_test);
            CATCH_REQUIRE(start_test == start_offset);
            CATCH_REQUIRE(end_test == end);
        }

        {
            vs.vseekg(is, 0, std::ios::beg);
            CATCH_REQUIRE(is.tellg() == start_offset);
            CATCH_REQUIRE(vs.vtellg(is) == 0);

            size_t const sz(std::min(max_read, FOUR));
            is.read(buf, sz);
            CATCH_REQUIRE(is.tellg() == static_cast<zipios::offset_t>(start_offset + sz));
            CATCH_REQUIRE(is);
            if(sz > 0)
            {
                CATCH_REQUIRE(buf[0] == static_cast<char>(start_offset));
            }
            if(sz > 1)
            {
                CATCH_REQUIRE(buf[1] == static_cast<char>(start_offset + 1));
            }
            if(sz > 2)
            {
                CATCH_REQUIRE(buf[2] == static_cast<char>(start_offset + 2));
            }
            if(sz > 3)
            {
                CATCH_REQUIRE(buf[3] == static_cast<char>(start_offset + 3));
            }

            // try moving a little more (if max_read allows it)
            if(max_read > 9UL)
            {
                vs.vseekg(is, 4, std::ios::cur);
                CATCH_REQUIRE(is.tellg() == start_offset + 8);
                CATCH_REQUIRE(vs.vtellg(is) == 8);

                size_t const sz2(std::min(max_read - 8UL, 4UL));
                is.read(buf, sz2);
                CATCH_REQUIRE(is);
                if(sz2 > 0)
                {
                    CATCH_REQUIRE(buf[0] == static_cast<char>(start_offset + 8));
                }
                if(sz2 > 1)
                {
                    CATCH_REQUIRE(buf[1] == static_cast<char>(start_offset + 8 + 1));
                }
                if(sz2 > 2)
                {
                    CATCH_REQUIRE(buf[2] == static_cast<char>(start_offset + 8 + 2));
                }
                if(sz2 > 3)
                {
                    CATCH_REQUIRE(buf[3] == static_cast<char>(start_offset + 8 + 3));
                }
            }
        }

        {
            ssize_t const sz(std::min(max_read, FOUR));

            vs.vseekg(is, -sz, std::ios::end);
            std::streampos const expected_absolute_pos(end_offset - sz);
            CATCH_REQUIRE(is.tellg() == expected_absolute_pos);
            std::streampos const expected_virtual_pos(end_offset - sz - start_offset);
            CATCH_REQUIRE(vs.vtellg(is) == expected_virtual_pos);

            is.read(buf, sz);
            CATCH_REQUIRE(is.tellg() == end_offset);
            CATCH_REQUIRE(is);
            if(sz > 0)
            {
                CATCH_REQUIRE(buf[0] == static_cast<char>(end_offset - sz));
            }
            if(sz > 1)
            {
                CATCH_REQUIRE(buf[1] == static_cast<char>(end_offset - sz + 1));
            }
            if(sz > 2)
            {
                CATCH_REQUIRE(buf[2] == static_cast<char>(end_offset - sz + 2));
            }
            if(sz > 3)
            {
                CATCH_REQUIRE(buf[3] == static_cast<char>(end_offset - sz + 3));
            }

            // try moving a little more (if max_read allows it)
            if(max_read >= 9UL && max_read - 8UL >= static_cast<size_t>(start_offset))
            {
                ssize_t const sz2(std::min(max_read - 8UL, 4UL));

                vs.vseekg(is, -sz2 - sz, std::ios::cur);
                std::streampos const expected_absolute_pos2(end_offset - sz2 - sz);
                CATCH_REQUIRE(is.tellg() == expected_absolute_pos2);
                std::streampos const expected_virtual_pos2(end_offset - sz2 - sz - start_offset);
                CATCH_REQUIRE(vs.vtellg(is) == expected_virtual_pos2);

                is.read(buf, sz2);
                CATCH_REQUIRE(is);
                if(sz2 > 0)
                {
                    CATCH_REQUIRE(buf[0] == static_cast<char>(end_offset - sz2 - sz));
                }
                if(sz2 > 1)
                {
                    CATCH_REQUIRE(buf[1] == static_cast<char>(end_offset - sz2 - sz + 1));
                }
                if(sz2 > 2)
                {
                    CATCH_REQUIRE(buf[2] == static_cast<char>(end_offset - sz2 - sz + 2));
                }
                if(sz2 > 3)
                {
                    CATCH_REQUIRE(buf[3] == static_cast<char>(end_offset - sz2 - sz + 3));
                }
            }
        }

        // change the offset and try again
        zipios::offset_t const start_offset2(rand() % 200);
        zipios::offset_t const end_offset2(start_offset2 + rand() % (256 - start_offset2));
        CATCH_REQUIRE(start_offset2 <= end_offset2); // this should not happen, period!
        zipios::offset_t const end2(256 - end_offset2);
        size_t max_read2(end_offset2 - start_offset2);
        // note that the "gap" may be zero

        // try setting the offsets with invalid values
        CATCH_REQUIRE_THROWS_AS(vs.setOffsets(-start_offset2, -end2), zipios::InvalidException);
        CATCH_REQUIRE_THROWS_AS(vs.setOffsets(start_offset2, -end2), zipios::InvalidException);
        if(start_offset2 != 0)
        {
            CATCH_REQUIRE_THROWS_AS(vs.setOffsets(-start_offset2, -end2), zipios::InvalidException);
        }

        // then change it to a valid value
        vs.setOffsets(start_offset2, end2);

        {
            CATCH_REQUIRE(vs.startOffset() == start_offset2);
            CATCH_REQUIRE(vs.endOffset() == end2);

            zipios::offset_t start_test2;
            zipios::offset_t end_test2;
            vs.getOffsets(start_test2, end_test2);
            CATCH_REQUIRE(start_test2 == start_offset2);
            CATCH_REQUIRE(end_test2 == end2);
        }

        for(int invalid_seek_direction(-100); invalid_seek_direction <= 100; ++invalid_seek_direction)
        {
            switch(invalid_seek_direction)
            {
            case std::ios::cur:
            case std::ios::beg:
            case std::ios::end:
                break;

            default:
                CATCH_REQUIRE_THROWS_AS(vs.vseekg(is, 0, static_cast<std::ios::seekdir>(invalid_seek_direction)), std::logic_error);
                break;

            }
        }

        {
            vs.vseekg(is, 0, std::ios::beg);
            CATCH_REQUIRE(vs.vtellg(is) == 0);

            size_t const sz(std::min(max_read2, FOUR));
            is.read(buf, sz);
            CATCH_REQUIRE(is);
            if(sz > 0)
            {
                CATCH_REQUIRE(buf[0] == static_cast<char>(start_offset2));
            }
            if(sz > 1)
            {
                CATCH_REQUIRE(buf[1] == static_cast<char>(start_offset2 + 1));
            }
            if(sz > 2)
            {
                CATCH_REQUIRE(buf[2] == static_cast<char>(start_offset2 + 2));
            }
            if(sz > 3)
            {
                CATCH_REQUIRE(buf[3] == static_cast<char>(start_offset2 + 3));
            }
        }

        {
            ssize_t const sz(std::min(max_read2, FOUR));

            vs.vseekg(is, -sz, std::ios::end);
            std::streampos const expected_absolute_pos(end_offset2 - sz);
            CATCH_REQUIRE(is.tellg() == expected_absolute_pos);
            std::streampos const expected_virtual_pos(end_offset2 - sz - start_offset2);
            CATCH_REQUIRE(vs.vtellg(is) == expected_virtual_pos);

            is.read(buf, sz);
            CATCH_REQUIRE(is);
            if(sz > 0)
            {
                CATCH_REQUIRE(buf[0] == static_cast<char>(end_offset2 - sz));
            }
            if(sz > 1)
            {
                CATCH_REQUIRE(buf[1] == static_cast<char>(end_offset2 - sz + 1));
            }
            if(sz > 2)
            {
                CATCH_REQUIRE(buf[2] == static_cast<char>(end_offset2 - sz + 2));
            }
            if(sz > 3)
            {
                CATCH_REQUIRE(buf[3] == static_cast<char>(end_offset2 - sz + 3));
            }
        }
    }
}


// Local Variables:
// mode: cpp
// indent-tabs-mode: nil
// c-basic-offset: 4
// tab-width: 4
// End:

// vim: ts=4 sw=4 et
