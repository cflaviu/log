#pragma once
#ifndef PCH
	#include <string>
	#include <boost/log/sources/severity_logger.hpp>
#endif

namespace stdext
{
	namespace log
	{
		enum app_area;

		std::basic_ostream<char>& operator << (std::basic_ostream<char>& strm, const app_area area);

		enum severity_level
		{
			fatal,
			error,
			warn,
			info,
			debug,
			trace,
		};

		typedef boost::log::sources::severity_logger_mt<severity_level> dispatcher_type;
		dispatcher_type& dispatcher();

		void init();
		void on_console(const bool show_file_location = true, const size_t area_text_width = 20);
		void on_debug_window(const bool show_file_location = true, const size_t area_text_width = 20);
		void on_csv_file(const std::string& appName, const std::string& logFolder, const uint8_t maxNoOfLogFiles, const size_t maxLogFileSize, const severity_level maxSeverityLevel, const char separator = '\t');
		void on_event_log(const std::string& appName);
	}
}

// specifies the area field in the log
#define LogArea(a)	(stdext::log::set_area(::log::a))

#define Log(level)	BOOST_LOG_FUNCTION();BOOST_LOG_STREAM_SEV(stdext::log::dispatcher(), stdext::log::##level)
