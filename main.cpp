#define BOOST_LOG_DYN_LINK 1
#include "log.h"
#include <thread>

namespace stdext
{
    namespace log
    {
        enum module_t: std::uint8_t
        {
            module1, // dummy module
        };

        const char* text_of(const module_t item) noexcept
        {
            static const char* const text[1]
            {
                "module1"
            };

            return text[item];
        }

        std::basic_ostream<char>& operator << (std::basic_ostream<char>& stream, const module_t module)
        {
            stream << text_of(module);
            return stream;
        }
    }
}

#define MODULE1_DEBUG GENERIC_LOG(stdext::log::module_t::module1, stdext::log::debug)
#define MODULE1_ERROR GENERIC_LOG(stdext::log::module_t::module1, stdext::log::error)

int main()
{
    using namespace stdext;
    log::init();
    log::on_csv_file("test", "", 3, 1024, log::trace);
    log::on_console();

    MODULE1_DEBUG << "Hello World!";
    MODULE1_ERROR << "An error";
    return 0;
}
