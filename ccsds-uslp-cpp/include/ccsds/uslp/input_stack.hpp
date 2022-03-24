#ifndef CCSDS_USLP_CPP_INCLUDE_CCSDS_USLP_INPUT_STACK_HPP_
#define CCSDS_USLP_CPP_INCLUDE_CCSDS_USLP_INPUT_STACK_HPP_

#include <map>
#include <memory>
#include <algorithm>

#include <ccsds/uslp/physical/abstract_pchannel.hpp>
#include <ccsds/uslp/master/abstract_mchannel.hpp>
#include <ccsds/uslp/virtual/abstract_vchannel.hpp>
#include <ccsds/uslp/map/abstract_map.hpp>

#include <ccsds/uslp/common/ids_io.hpp>


namespace ccsds { namespace uslp {


class input_stack_event_handler
{
public:
	input_stack_event_handler() = default;
	virtual ~input_stack_event_handler() = default;

	void dispatch_event(const acceptor_event & event);

protected:
	virtual void _on_map_sdu_event(const acceptor_event_map_sdu & event) {}
};


class input_stack
{
public:
	input_stack();

	void set_event_handler(input_stack_event_handler * event_handler);

	void push_frame(const uint8_t * frame_data, size_t frame_data_size);

	template<typename T, typename... ARGS>
	T * create_map(gmapid_t mapid, ARGS && ...args);

	template<typename T, typename... ARGS>
	T * create_vchannel(gvcid_t gvcid, ARGS && ...args);

	template<typename T, typename... ARGS>
	T * create_mchannel(mcid_t mcid, ARGS && ...args);

	template<typename T, typename... ARGS>
	T * create_pchannel(std::string name, ARGS && ...args);

	void finalize();

	void dispatch_event(const acceptor_event & event);

private:
	input_stack_event_handler * _event_handler;

	std::map<gmapid_t, std::unique_ptr<map_acceptor>> _maps;
	std::map<gvcid_t, std::unique_ptr<vchannel_acceptor>> _virtuals;
	std::map<mcid_t, std::unique_ptr<mchannel_acceptor>> _masters;
	std::unique_ptr<pchannel_acceptor> _pchannel;
};


template<typename T, typename... ARGS>
T * input_stack::create_map(gmapid_t mapid, ARGS && ...args)
{
	auto itt = _virtuals.find(mapid.gvcid());
	if (itt == _virtuals.end())
	{
		std::stringstream error;
		error << "unable to find virtual channel sink for map source " << mapid;
		throw std::logic_error(error.str());
	}

	std::unique_ptr<T> map(new T(
			this, std::move(mapid), std::forward<ARGS>(args)...
	));

	auto * retval = map.get();
	itt->second->add_map_accceptor(retval);
	_maps.emplace(mapid, std::move(map));
	return retval;
}


template<typename T, typename... ARGS>
T * input_stack::create_vchannel(gvcid_t gvcid, ARGS && ...args)
{
	auto itt = _masters.find(gvcid.mcid());
	if (itt == _masters.end())
	{
		std::stringstream error;
		error << "unable to find mchannel sink for vchannel " << gvcid;
		throw std::logic_error(error.str());
	}

	std::unique_ptr<T> vchannel(new T(
			this, std::move(gvcid), std::forward<ARGS>(args)...
	));

	auto * retval = vchannel.get();
	itt->second->add_vchannel_acceptor(retval);
	_virtuals.emplace(gvcid, std::move(vchannel));
	return retval;
}


template<typename T, typename... ARGS>
T * input_stack::create_mchannel(mcid_t mcid, ARGS && ...args)
{
	if (!_pchannel)
		throw std::logic_error("pchannel sink should be created before mchannel sinks in input stack");

	std::unique_ptr<T> mchannel(new T(
			this, std::move(mcid), std::forward<ARGS>(args)...
	));

	auto * retval = mchannel.get();
	_pchannel->add_mchannel_acceptor(retval);
	_masters.emplace(mcid, std::move(mchannel));
	return retval;
}


template<typename T, typename... ARGS>
T * input_stack::create_pchannel(std::string name, ARGS && ...args)
{
	if (_pchannel)
		throw std::logic_error("unable to create second pchannel sink in input stack");

	auto * retval = new T(this, std::move(name), std::forward<ARGS>(args)...);
	_pchannel.reset(retval);
	return retval;
}


}}


#endif /* CCSDS_USLP_CPP_INCLUDE_CCSDS_USLP_INPUT_STACK_HPP_ */
