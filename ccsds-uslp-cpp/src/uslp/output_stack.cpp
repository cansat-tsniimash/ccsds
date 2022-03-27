#include <ccsds/uslp/output_stack.hpp>

#include <cassert>

#include <ccsds/common/exceptions.hpp>


namespace ccsds { namespace uslp {


static output_stack_event_handler _default_event_handler;


// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-


void output_stack_event_handler::dispatch_event(const emitter_event & event)
{
	switch(event.kind)
	{
	case emitter_event::kind_t::SDU_EMITTED:
		_on_frame_emitted(dynamic_cast<const emitter_event_sdu_emitted&>(event));
		break;

	default:
		assert(false);
	}
}


// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-


output_stack::output_stack()
{
	set_event_handler(&_default_event_handler);
}


bool output_stack::peek_frame()
{
	if (!_pchannel)
		throw einval_exception("unable to peek output stack frame without pchannel in it");

	return _pchannel->peek();
}


bool output_stack::peek_frame(pchannel_frame_params_t & frame_params)
{
	if (!_pchannel)
		throw einval_exception("unable to peek output stack frame without pchannel in it");

	return _pchannel->peek(frame_params);
}


void output_stack::pop_frame(uint8_t * frame_buffer, size_t frame_buffer_size)
{
	if (!_pchannel)
		throw einval_exception("unable to pop output stack frame without pchannel in it");

	_pchannel->pop(frame_buffer, frame_buffer_size);
}


void output_stack::finalize()
{
	assert(_pchannel);
	_pchannel->finalize();
}


void output_stack::dispatch_event(const emitter_event & event)
{
	_event_handler->dispatch_event(event);
}


map_emitter * output_stack::get_map_channel(const gmapid_t & id)
{
	auto itt = _maps.find(id);
	if (_maps.end() == itt)
		return nullptr;

	return itt->second.get();
}


}}
