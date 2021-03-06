#ifndef INCLUDE_CCSDS_USLP_VIRTUAL_MAP_RR_MUXER_HPP_
#define INCLUDE_CCSDS_USLP_VIRTUAL_MAP_RR_MUXER_HPP_

#include <ccsds/uslp/virtual/abstract_vchannel.hpp>
#include <ccsds/uslp/map/abstract_map.hpp>
#include <ccsds/uslp/common/rr_muxer.hpp>


namespace ccsds { namespace uslp {


class map_rr_muxer: public vchannel_emitter
{
public:
	map_rr_muxer(output_stack * stack, gvcid_t gvcid_);

protected:
	virtual void add_map_emitter_impl(map_emitter * emitter) override;

	virtual void finalize_impl() override;

	virtual bool peek_impl() override;
	virtual bool peek_impl(vchannel_frame_params & params) override;
	virtual void pop_impl(uint8_t * tfdf_buffer) override;

private:
	struct map_status_checker_t
	{
		const bool operator()(map_emitter * source) const {return source->peek_tfdf(); }
	};

	typedef rr_muxer<map_emitter*, map_status_checker_t> muxer_t;

	muxer_t _muxer;
	map_emitter * _selected_channel = nullptr;
};

}}


#endif /* INCLUDE_CCSDS_USLP_VIRTUAL_MAP_RR_MUXER_HPP_ */
