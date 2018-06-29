#pragma once

#include <iostream>
#include <sstream>

namespace frequency_scaling {

    /**
        * @brief The full_expression_accumulator class
        */
    class full_expression_accumulator {
    public:
        explicit full_expression_accumulator(std::ostream &os) : os_(os), flushed_(false) {}

        ~full_expression_accumulator() {
            if (!flushed_) {
                // single call to std::cout << is threadsafe (c++11)
                os_ << ss_.str() << std::flush;
            }
        }

        /**
         *
         */
        template<typename T>
        full_expression_accumulator &operator<<(const T &t) {
            // accumulate into a non-shared stringstream, no threading issues
            ss_ << t;
            return *this;
        }


        full_expression_accumulator &operator<<(std::ostream &(*io_manipulator)(std::ostream &)) {
            ss_ << io_manipulator;
            return *this;
        }

        full_expression_accumulator &operator<<(std::basic_ios<char> &(*io_manipulator)(std::basic_ios<char> &)) {
            ss_ << io_manipulator;
            return *this;
        }

        full_expression_accumulator &operator<<(std::ios_base &(*io_manipulator)(std::ios_base &)) {
            ss_ << io_manipulator;
            return *this;
        }


        void flush() {
            // single call to std::cout << is threadsafe (c++11)
            os_ << ss_.str() << std::flush;
            flushed_ = true;
        }


    private:
        std::ostream &os_;
        std::ostringstream ss_;
        bool flushed_;
    };

}

