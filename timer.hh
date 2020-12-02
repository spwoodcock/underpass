//
// Copyright (c) 2020, Humanitarian OpenStreetMap Team
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

#ifndef __TIMER_HH__
#define __TIMER_HH__

// This is generated by autoconf
#ifdef HAVE_CONFIG_H
# include "hotconfig.h"
#endif

#include <string>
#include <vector>
#include <iostream>
#include <iomanip>

#include <boost/date_time.hpp>
#include "boost/date_time/posix_time/posix_time.hpp"
using namespace boost::posix_time;
using namespace boost::gregorian;

/// \file timer.hh
/// \brief Implement a timer for performance testing

/// \class Timer
/// \brief create a performance timer
///
/// This implements a simple timer used for performance testing
/// during development.
class Timer
{
public:
    /// Start a timer, only used for performance analysis
    void startTimer(void) {
        start = boost::posix_time::microsec_clock::local_time();
    };
    /// Stop the timer, used for performance analysis
    long endTimer(void) {
        return endTimer("");
    };
    long endTimer(const std::string &msg) {
        end = boost::posix_time::microsec_clock::local_time();
        boost::posix_time::time_duration delta = end - start;
        average += delta.total_seconds() + (delta.total_milliseconds()/1000);
        if (interval >= counter || interval == 0) {
            // if (interval > 0) {
            //     std::cout << msg << ": Operation took " << average/interval << " milliseconds" << std::endl;
            // } else {
            std::cout << msg << ": Operation took " << std::setprecision(3)
                //<< std::to_string(delta.total_seconds())
                      << (double)delta.total_milliseconds()/1000
                      << " seconds" << std::endl;
            // }
            
            counter = 0;
            average = 0;
        }
        counter++;
        return delta.total_milliseconds();
    };

    /// Set the interval for printing results
    void setInterval(int x) { interval = x; }; 
private:   
    // These are just for performance testing
    ptime start;                ///< Starting timestamop for operation
    ptime end;                  ///< Ending timestamop for operation
    int interval = 0;           ///< Time Interval for long running commands
    int counter = 0;            ///< counter for printing collected statistics
    double average = 0.0;           ///< The average time in each interval
};


#endif  // EOF __TIMER_HH__
