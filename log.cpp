//#include "pch.h"
#include "log.h"
#ifndef PCH
	#include <boost/date_time/posix_time/posix_time_types.hpp>
	#include <boost/log/core.hpp>
	#include <boost/log/attributes.hpp>
	#include <boost/log/expressions.hpp>
	#include <boost/log/sinks.hpp>
	#include <boost/log/utility/setup/file.hpp>
	#include <boost/core/null_deleter.hpp>
	#include <boost/log/sources/severity_feature.hpp>
	#include <iomanip>
	#include <iostream>
	#include "this_thread_id.hpp"
#endif

#ifdef _WIN32
#pragma warning (disable:4503)
#endif

namespace lg = boost::log;
namespace src = boost::log::sources;
namespace sinks = boost::log::sinks;
namespace keyword = boost::log::keywords;
namespace expr = boost::log::expressions;
namespace attrs = boost::log::attributes;

namespace stdext {
namespace log {

template <typename date_time_type>
void format_time_ms(std::ostringstream& formatter, const date_time_type& date_time)
{
	auto time = date_time.time_of_day();
	using namespace std;
	formatter
		<< setfill('0') << setw(2) << time.hours() << ':'
		<< setfill('0') << setw(2) << time.minutes() << ':'
		<< setfill('0') << setw(2) << time.seconds() << ','
		<< setfill('0') << setw(3) << time.fractional_seconds() / 1000;
}

template <typename date_time_type>
std::string format_time_ms(const date_time_type& date_time)
{
	std::ostringstream formatter;
	format_time_ms(formatter, date_time);
	return formatter.str();
}

template <typename date_time_type>
std::string format_date_time_ms(const date_time_type& date_time, const char date_time_sep = ' ')
{
	using namespace std;
	ostringstream formatter;
	auto date = date_time.date();
	formatter
		<< date.year() << '-'
		<< setfill('0') << setw(2) << int(date.month()) << '-'
		<< setfill('0') << setw(2) << date.day() << date_time_sep;
	format_time_ms(formatter, date_time);
	return formatter.str();
}

template <typename date_time_type, const char date_time_sep = ' '>
struct date_time_ms_formatter
{
	std::string operator () (const date_time_type& date_time) { return format_date_time_ms(date_time, date_time_sep); }
};

struct time_ms_formatter
{
	template <typename date_time_type>
	std::string operator () (const date_time_type& date_time) { return format_time_ms(date_time); }
};

template <typename time_type>
struct local_clock_source
{
	time_type operator () () const
	{
		return boost::date_time::microsec_clock<time_type>::local_time();
	}
};

template <typename time_type>
struct universal_clock_source
{
	time_type operator () () const
	{
		return boost::date_time::microsec_clock<time_type>::universal_time();
	}
};

template <typename time_type, typename clock_source_type, typename formater_type>
class custom_clock : public lg::attribute
{
public:
	class impl : public lg::attribute::impl
	{
	public:
		lg::attribute_value get_value()
		{
			auto str = formater_type()(clock_source_type()());
			return attrs::make_attribute_value(str);
		}
	};

	custom_clock() : attribute(new impl()) {}

	explicit custom_clock(const attrs::cast_source& source) : lg::attribute(source.as<impl>()) {}
};

using ptime = boost::posix_time::ptime;

using local_date_time_ms_clock = custom_clock<ptime, local_clock_source<ptime>, date_time_ms_formatter<ptime, '\t'>	>;
using universal_date_time_ms_clock = custom_clock<ptime, universal_clock_source<ptime>, date_time_ms_formatter<ptime, '\t'> >;

using local_time_ms_clock = custom_clock<ptime, local_clock_source<ptime>, time_ms_formatter>;
using universal_time_ms_clock = custom_clock<ptime, universal_clock_source<ptime>, time_ms_formatter>	;

BOOST_LOG_ATTRIBUTE_KEYWORD(log_no, "No", unsigned)
BOOST_LOG_ATTRIBUTE_KEYWORD(datetime_stamp, "DateTime", boost::posix_time::ptime)
BOOST_LOG_ATTRIBUTE_KEYWORD(time_stamp, "Time", boost::posix_time::ptime)
BOOST_LOG_ATTRIBUTE_KEYWORD(crt_thread_id, "ThreadID", attrs::this_thread_id::value_type)
BOOST_LOG_ATTRIBUTE_KEYWORD(severity, "Severity", severity_t)
BOOST_LOG_ATTRIBUTE_KEYWORD(scope, "Scope", attrs::named_scope_list)
BOOST_LOG_ATTRIBUTE_KEYWORD(module, "Module", module_t)

attrs::mutable_constant<module_t> module_field(module_t(0));

void set_module(const module_t m)
{
	module_field.set(m);
}

template <typename CharT, typename TraitsT>
std::basic_ostream<CharT, TraitsT>& operator << (std::basic_ostream<CharT, TraitsT>& strm, const severity_t id)
{
	static const char* const text[] =
	{
		"Fatal",
		"Error",
		"Warn",
		"Info",
		"Debug",
		"Trace"
	};

	return strm << text[id];
}

void write_header(sinks::text_file_backend::stream_type& file)
{
	file << "Log No\tDate\tTime\tThread ID\tSeverity\tModule\tMessage\tSource File\tLine No\n";
}

void write_footer(sinks::text_file_backend::stream_type& file)
{
	file << "end ---------------------------------------------------------------------------------------------";
}

constexpr char sp = ' ';

using log_file_sink_t = sinks::synchronous_sink<sinks::text_file_backend>;
boost::shared_ptr<log_file_sink_t> log_file_sink;

void on_csv_file(const std::string& appName, const std::string& log_folder, const uint8_t max_no_of_log_flags,
				 const size_t max_log_file_size, const severity_t max_severity_level, const char separator)
{
	if (log_file_sink)
	{
		lg::core::get()->remove_sink(log_file_sink);
	}

	auto log_file_backend = boost::make_shared<sinks::text_file_backend>
	(
		keyword::target = log_folder,
		keyword::file_name = log_folder + appName + "_%N.log",
		keyword::open_mode = (std::ios::out | std::ios::app),
		keyword::auto_flush = true,
		keyword::scan_method = sinks::file::scan_all,
		keyword::rotation_size = max_log_file_size
	);

	log_file_sink = boost::make_shared<log_file_sink_t>(log_file_backend);
	log_file_sink->set_filter(severity <= max_severity_level);
	log_file_sink->set_formatter(expr::stream
		<< std::setw(5) << std::setfill('0') << log_no << separator
		<< expr::attr<std::string>(datetime_stamp.get_name()) << separator
		<< std::setw(3) << std::setfill('0') << crt_thread_id << separator
		<< module << separator << severity << separator
		//<< expr::format_named_scope( scope, keyword::format = "%f\t%l") << separator
		<< expr::format_named_scope(scope,
			keyword::format = "%f\t%l",
			//keyword::iteration = boost::log::expressions::reverse,
			keyword::incomplete_marker = "",
			keyword::depth = 1) << separator
		<< expr::csv_decor[expr::stream << expr::message]
	);

	auto backend = log_file_sink->locked_backend();
	//backend->set_open_handler( &write_header);
	//backend->set_close_handler( &write_footer);
	backend->set_file_collector(sinks::file::make_collector
	(
		keyword::target = log_folder,
		keyword::max_size = max_no_of_log_flags * max_log_file_size
	));

	lg::core::get()->add_sink(log_file_sink);
}

void set_level(severity_t max_severity_level)
{
	if (log_file_sink)
	{
		log_file_sink->set_filter(severity <= max_severity_level);
	}
}

void on_console(const bool show_file_location, const size_t module_text_width)
{
	using console_sing_t = sinks::synchronous_sink<sinks::text_ostream_backend>;

	auto sink = boost::make_shared<console_sing_t>();
	sink->locked_backend()->add_stream(boost::shared_ptr<std::ostream>(&std::cout, boost::null_deleter()));
	auto formatter = expr::stream << std::left
		<< expr::attr<std::string>(time_stamp.get_name()) << sp
		<< '[' << std::setw(3) << std::setfill('0') << crt_thread_id << ']' << sp << std::setw(6)
		<< std::setfill(' ') << std::setw(module_text_width) << module << sp << '<' << severity << '>'
		<< sp << expr::smessage << sp
		<< expr::format_named_scope(scope,
			keyword::format = (show_file_location ? "[%F:%l @ %c]" : ""),
			//keyword::iteration = boost::log::expressions::reverse,
			keyword::incomplete_marker = "",
			keyword::depth = 1);
	sink->locked_backend()->auto_flush(true);
	sink->set_formatter(formatter);
	lg::core::get()->add_sink(sink);
}

#ifdef _WIN32
	void on_event_log(const std::string& appName)
	{
		using event_log_sink_t = sinks::synchronous_sink<sinks::simple_event_log_backend> ;

		auto sink = boost::make_shared<event_log_sink_t>(keyword::log_source = appName);
		sink->set_formatter(expr::stream << '[' << module << ']' << sp << expr::smessage);

		sinks::event_log::custom_event_type_mapping<severity_level> mapping(severity.get_name());
		mapping[fatal] = sinks::event_log::error;
		mapping[error] = sinks::event_lo Avatar
				😁
				Alan de Freitasg::error;
		mapping[warn] = sinks::event_log::warning;
		mapping[info] = sinks::event_log::info;
		mapping[debug] = sinks::event_log::info;
		mapping[trace] = sinks::event_log::info;

		sink->locked_backend()->set_event_type_mapper(mapping);
		sink->set_filter(severity < info);
		lg::core::get()->add_sink(sink);
	}

	void on_debug_window(const bool show_file_location, const size_t module_text_width)
	{
		using debug_sink_t = sinks::synchronous_sink<sinks::debug_output_backend>;

		auto sink = boost::make_shared<debug_sink_t>();
		sink->set_filter(expr::is_debugger_present());
		auto formatter = expr::stream << std::left
			<< expr::attr<std::string>(time_stamp.get_name()) << sp
			<< '[' << crt_thread_id << ']' << sp << std::setw(6) << severity << '{' << std::setw(module_text_width) << module << '}'
			<< expr::format_named_scope(scope,
				keyword::format = (show_file_location ? "[%f : %l]" : ""),
				//keyword::iteration = boost::log::expressions::reverse,
				keyword::incomplete_marker = "",
				keyword::depth = 1) << sp
			<< expr::smessage << std::endl;
		sink->set_formatter(formatter);
		lg::core::get()->add_sink(sink);
	}
#endif

dispatcher_type& dispatcher()
{
	static dispatcher_type item;
	return item;
}

void init()
{
	auto core = lg::core::get();
	core->add_global_attribute(log_no.get_name(), attrs::counter<unsigned>(1));
	core->add_global_attribute(datetime_stamp.get_name(), local_date_time_ms_clock());
	core->add_global_attribute(time_stamp.get_name(), local_time_ms_clock());
	core->add_global_attribute(crt_thread_id.get_name(), attrs::this_thread_id());
	core->add_global_attribute(module.get_name(), module_field);
	core->add_global_attribute(scope.get_name(), attrs::named_scope());
}

}}
