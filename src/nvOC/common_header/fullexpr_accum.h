#pragma once

#include <iostream>
#include <sstream>

namespace frequency_scaling {

    /**
        * @brief The full_expression_accumulator class
        */
    template<typename OUT_STREAM = std::ostream>
    class fullexpr_accum {
    public:
        explicit fullexpr_accum(OUT_STREAM &os, bool log_cond = true) : os_(os), flushed_(!log_cond) {}

        ~fullexpr_accum() {
            if (!flushed_) {
                // single call to std::cout << is threadsafe (c++11)
                os_ << ss_.str() << std::flush;
            }
        }


        template<typename T>
        fullexpr_accum &operator<<(const T &t) {
            // accumulate into a non-shared stringstream, no threading issues
            ss_ << t;
            return *this;
        }


        fullexpr_accum &operator<<(std::ostream &(*io_manipulator)(std::ostream &)) {
            ss_ << io_manipulator;
            return *this;
        }

        fullexpr_accum &operator<<(std::basic_ios<char> &(*io_manipulator)(std::basic_ios<char> &)) {
            ss_ << io_manipulator;
            return *this;
        }

        fullexpr_accum &operator<<(std::ios_base &(*io_manipulator)(std::ios_base &)) {
            ss_ << io_manipulator;
            return *this;
        }


        void flush() {
            if (!flushed_) {
                // assumption: single call to os_ << is threadsafe
                os_ << ss_.str() << std::flush;
                flushed_ = true;
            }
        }

    private:
        OUT_STREAM &os_;
        std::ostringstream ss_;
        bool flushed_;
    };

}

