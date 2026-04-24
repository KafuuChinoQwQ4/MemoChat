#include <exception>
#include <boost/assert/source_location.hpp>
#include <boost/config.hpp>

namespace boost {

BOOST_NORETURN void throw_exception(std::exception const& e) {
    throw e;
}

BOOST_NORETURN void throw_exception(std::exception const& e, boost::source_location const&) {
    throw e;
}

}
