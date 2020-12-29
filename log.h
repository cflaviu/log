#pragma once
#define BOOST_LOG_DYN_LINK 1
#ifndef PCH
    #include <string>
    #include <boost/log/common.hpp>
    #include <boost/log/sources/severity_logger.hpp>
#endif

namespace stdext
{
    namespace log
    {
        // forward declaration allowing customization
        enum module_t: std::uint8_t;

        std::basic_ostream<char>& operator << (std::basic_ostream<char>& strm, const module_t m);

        enum severity_t
        {
            fatal,
            error,
            warn,
            info,
            debug,
            trace,
        };

		using dispatcher_type = boost::log::sources::severity_logger_mt<severity_t>;
		dispatcher_type& dispatcher();

		void init();
		void on_console(bool show_file_location = true, const size_t module_text_width = 20);
		void on_csv_file(const std::string& appName, const std::string& log_folder, const uint8_t max_no_of_log_files, const size_t max_log_file_size,
						 const severity_t max_severity_level = trace, const char separator = '\t');

		#ifdef _WIN32
			void on_event_log(const std::string& appName);
			void on_debug_window(const bool show_file_location = true, const size_t area_text_width = 20);
		#endif

		void set_module(module_t m);
	}
}

#define GENERIC_LOG(module,level) BOOST_LOG_FUNCTION();set_module(module);BOOST_LOG_STREAM_SEV(::stdext::log::dispatcher(),level)
