#ifndef INCLUDE_CCSDS_USLP_EXCEPTIONS_HPP_
#define INCLUDE_CCSDS_USLP_EXCEPTIONS_HPP_


#include <stdexcept>
#include <string>


namespace ccsds {

	class exception: public std::runtime_error
	{
	public:
		exception() = default;
		exception(const char * what);
		exception(const std::string & what);
		virtual ~exception() = default;
	};


	class einval_exception: public exception
	{
	public:
		einval_exception() = default;
		einval_exception(const char * what);
		einval_exception(const std::string & what);
		virtual ~einval_exception() = default;
	};


	class invalid_call_order: public exception
	{
	public:
		invalid_call_order() = default;
		invalid_call_order(const char * what);
		invalid_call_order(const std::string & what);
		virtual ~invalid_call_order() = default;
	};


	class object_is_finalized: public exception
	{
	public:
		object_is_finalized() = default;
		object_is_finalized(const char * what);
		object_is_finalized(const std::string & what);
		virtual ~object_is_finalized() = default;
	};


	class not_implemented: public exception
	{
	public:
		not_implemented() = default;
		not_implemented(const char * what);
		not_implemented(const std::string & what);
		virtual ~not_implemented() = default;
	};
}



#endif /* INCLUDE_CCSDS_USLP_EXCEPTIONS_HPP_ */
