#ifndef INCLUDE_CCSDS_USLP_MAP_ABSTRACT_MAP_HPP_
#define INCLUDE_CCSDS_USLP_MAP_ABSTRACT_MAP_HPP_

#include <cstdint>
#include <functional>
#include <vector>
#include <optional>

#include <ccsds/uslp/common/defs.hpp>
#include <ccsds/uslp/common/ids.hpp>
#include <ccsds/uslp/common/frame_seq_no.hpp>

#include <ccsds/uslp/events.hpp>


namespace ccsds { namespace uslp {


class output_stack;
class input_stack;


//! Параметры пейлоада, отправляемого этим map каналом
struct output_map_frame_params
{
	//! Тип гарантии доставки этого кадра
	qos_t qos;
	//! Можно ли переходить к следующему map каналу после отправки этого фрейма
	bool channel_lock;
	//! Список кукисов юнитов полезной нагрузки, которые едут в этом фрейме
	std::vector<payload_cookie_t> payload_cookies;
};


class map_emitter
{
public:
	map_emitter(output_stack * stack, gmapid_t map_id_);
	virtual ~map_emitter() = default;

	output_stack & stack() { return *_stack; }

	void tfdf_size(uint16_t value);
	uint16_t tfdf_size() const noexcept { return _tfdf_size; }

	void finalize();

	bool peek_tfdf();
	bool peek_tfdf(output_map_frame_params & params);
	void pop_tfdf(uint8_t * tfdf_buffer, uint16_t tfdf_buffer_size);

	const gmapid_t channel_id;

protected:
	void emit_event(const emitter_event & evt);

	virtual void finalize_impl();

	virtual bool peek_tfdf_impl() = 0;
	virtual bool peek_tfdf_impl(output_map_frame_params & params) = 0;
	virtual void pop_tfdf_impl(uint8_t * tfdf_buffer) = 0;

private:
	output_stack * _stack;
	bool _finalized = false;
	uint16_t _tfdf_size = 0;
};


//! Параметры входящего мап каналва
struct input_map_frame_params
{
	//! тип гарантии доставки этого кадра
	qos_t qos;
	//! Номер фрейма
	std::optional<frame_seq_no_t> frame_seq_no;
};


class map_acceptor
{
public:
	map_acceptor(input_stack * stack, gmapid_t map_id_);
	virtual ~map_acceptor() = default;

	input_stack & stack() { return *_stack; }

	void finalize();

	void push(
			const input_map_frame_params & params,
			const uint8_t * tfdf_buffer, uint16_t tfdf_buffer_size
	);

	const gmapid_t channel_id;

protected:
	void emit_event(const acceptor_event & event);

	virtual void finalize_impl();
	virtual void push_impl(
			const input_map_frame_params & params,
			const uint8_t * tfdf_buffer, uint16_t tfdf_buffer_size
	) = 0;

	bool is_finalized() const { return _finalized; }

private:
	input_stack * _stack;
	bool _finalized = false;
};


}}


#endif /* INCLUDE_CCSDS_USLP_MAP_ABSTRACT_MAP_HPP_ */
