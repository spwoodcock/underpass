//
// Copyright (c) 2020, 2021, 2022 Humanitarian OpenStreetMap Team
//
// This file is part of Underpass.
//
//     Underpass is free software: you can redistribute it and/or modify
//     it under the terms of the GNU General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Underpass is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU General Public License for more details.
//
//     You should have received a copy of the GNU General Public License
//     along with Underpass.  If not, see <https://www.gnu.org/licenses/>.
//

#ifndef __QUERYVALIDATE_HH__
#define __QUERYVALIDATE_HH__

/// \file queryvalidate.hh
/// \brief This file is used to work with the OSM Validation database
///
/// This manages the OSM Validation schema in a postgres database. This
/// includes querying existing data in the database, as well as
/// updating the database.

// This is generated by autoconf
#ifdef HAVE_CONFIG_H
#include "unconfig.h"
#endif

#include <algorithm>
#include <array>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <boost/date_time.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
using namespace boost::posix_time;
using namespace boost::gregorian;

#include "validate/validate.hh"
#include "osm/changeset.hh"
#include "osm/osmchange.hh"

#include "data/pq.hh"

using namespace pq;

// Forward declarations
namespace osmchange {
  class OsmChange;
}; // namespace osmchange

/// \namespace queryvalidate
namespace queryvalidate {

/// \class QueryValidate
/// \brief This build validation queries for the database
///
/// This manages the OSM Validation schema in a postgres database.
/// This includes building queries for existing data in the database,
/// as well for updating the database.

class QueryValidate  {
  public:
    QueryValidate(void);
    ~QueryValidate(void){};
    QueryValidate(std::shared_ptr<Pq> db);

    /// Apply data validation to the database
    std::string applyChange(const ValidateStatus &validation) const;
    /// Update the validation table, delete any feature that has been fixed.
    std::string updateValidation(std::shared_ptr<std::vector<long>> removals);
    // Database connection, used for escape strings
    std::shared_ptr<Pq> dbconn;
};

} // namespace queryvalidate

#endif // EOF __QUERYVALIDATE_HH__

// local Variables:
// mode: C++
// indent-tabs-mode: nil
// End:
