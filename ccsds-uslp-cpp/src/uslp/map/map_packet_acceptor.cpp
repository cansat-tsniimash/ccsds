#include <ccsds/uslp/map/map_packet_acceptor.hpp>

#include <sstream>
#include <cassert>
#include <array>

#include <ccsds/common/exceptions.hpp>
#include <ccsds/uslp/_detail/tfdf_header.hpp>


namespace ccsds { namespace uslp {


map_packet_acceptor::map_packet_acceptor(input_stack * stack, gmapid_t channel_id)
	: map_acceptor(stack, channel_id), _prev_frame_qos()
{

}


void map_packet_acceptor::max_packet_size(size_t value)
{
	if (is_finalized())
	{
		std::stringstream error;
		error << "Can`t use max_packet_size on map_packet_sink, because it is finalized";
		throw object_is_finalized(error.str());
	}

	_max_packet_size = value;
}


void map_packet_acceptor::emit_idle_packets(bool value)
{
	if (is_finalized())
	{
		std::stringstream error;
		error << "Can`t use emit_idle_packets on map_packet_sink, because it is finalized";
		throw object_is_finalized(error.str());
	}

	map_packet_acceptor::_emit_idle_packets = value;
}


void map_packet_acceptor::emit_stray_data(bool value)
{
	if (is_finalized())
	{
		std::stringstream error;
		error << "Can`t use emit_idle_packets on map_packet_sink, because it is finalized";
		throw object_is_finalized(error.str());
	}

	map_packet_acceptor::_emit_stray_data = value;
}


void map_packet_acceptor::finalize_impl()
{
	map_acceptor::finalize_impl();
}


void map_packet_acceptor::push_impl(
		const input_map_frame_params & params,
		const uint8_t * tfdf_buffer, uint16_t tfdf_buffer_size
)
{
	if (detail::tfdf_header_t::full_size >= tfdf_buffer_size)
	{
		// Этот фрейм не может быть валидным, так как в него ничего не влезает
		_flush_accum(acceptor_event_map_sdu::INCOMPLETE);
		_prev_frame_qos = params.qos;
		_prev_frame_seq_no = params.frame_seq_no;
		return;
	}

	// Читаем заголовок tfdf
	detail::tfdf_header_t tfdf_header;
	tfdf_header.read(tfdf_buffer, true);
	if (tfdf_header.ctr_rule != detail::tfdz_construction_rule_t::PACKETS_SPANNING_MULTIPLE_FRAMES)
	{
		// Это не MAPP данные!
		// сбрасываем накопленное
		_flush_accum(acceptor_event_map_sdu::INCOMPLETE);
		return;
	}

	const uint8_t * const tfdz_start = tfdf_buffer + tfdf_header.size();
	const size_t tfdz_size = tfdf_buffer_size - tfdf_header.size();
	const uint8_t * const tfdz_end = tfdz_start + tfdz_size;

again:
	// Если мы пустые
	if (_accumulator.empty())
	{
		// Смотрим есть ли в этом фрейме начало пакета
		assert(tfdf_header.first_header_offset.has_value());
		if (tfdf_header.first_header_offset.value() > tfdz_size)
		{
			// Нету. Ну... искать не будем
			_process_stay(tfdz_start, tfdz_end);
		}
		else
		{
			// А вот если есть - тут по-подробнее
			const uint8_t * const stray_start = tfdz_start;
			const uint8_t * const stray_end = tfdz_start + tfdf_header.first_header_offset.value();
			_process_stay(stray_start, stray_end);

			const uint8_t * const packet_start = tfdz_start + tfdf_header.first_header_offset.value();
			const uint8_t * const packet_part_end = tfdz_start + tfdz_size;
			// кушаем с начала первого пакета, а там разберемся
			_consume_bytes(packet_start, packet_part_end);
		}
	}
	else
	{
		// Мы не пустые. Проверяем последовательность фреймов
		if (_prev_frame_seq_no.has_value() != params.frame_seq_no.has_value())
		{
			// Ой, так быть не должно
			_flush_accum(acceptor_event_map_sdu::INCOMPLETE);
			goto again;
		}
		else if (_prev_frame_seq_no.value() + 1 != params.frame_seq_no.value())
		{
			// Ой. Так тоже быть не должно
			_flush_accum(acceptor_event_map_sdu::INCOMPLETE);
			goto again;
		}
		else if (_prev_frame_qos != params.qos)
		{
			// Ну и так тоже не должно быть
			_flush_accum(acceptor_event_map_sdu::INCOMPLETE);
			goto again;
		}

		// Кушаем этот фрейм целиком. а там разберемся
		_consume_bytes(tfdz_start, tfdz_start + tfdz_size);
	}

	_prev_frame_qos = params.qos;
	_prev_frame_seq_no = params.frame_seq_no;

	// А теперь пробуем разгрести что нашлось
	_parse_packets();
}


void map_packet_acceptor::_consume_bytes(const uint8_t * begin, const uint8_t * end)
{
	// Ну, на последовательность все проверяли выше
	// Поэтому мы просто жрем все байты в наш буфер
	std::copy(begin, end, std::back_inserter(_accumulator));
}


void map_packet_acceptor::_parse_packets()
{
again:
	if (_accumulator.empty())
		return; // Ну тут все

	if (!_current_packet_header.has_value())
	{
		// Если у нас еще нет заголовка
		// Смотрим хватает ли у нас на него байт
		// Мы работаем только с epp пакетами
		const auto header_size = epp::header_t::probe_header_size(_accumulator.front());
		if (0 == header_size)
		{
			// Значит это не заголовок... Сбрасывем все
			_flush_accum(acceptor_event_map_sdu::INCOMPLETE);
			return;
		}

		// У нас уже достаточно байт на заголовок?
		if (_accumulator.size() < header_size)
			return; // Ну, будем подождать

		// Отлично, пробуем разобрать
		epp::header_t epp_header;
		try
		{
			epp_header.read(_accumulator.begin(), _accumulator.end());
		}
		catch (std::exception & e)
		{
			// К сожалению - весь наш буфер уходит в труху, потому что мы не можем найти
			// границы никакого пакета
			_flush_accum(acceptor_event_map_sdu::INCOMPLETE);
			return;
		}

		// Теперь у нас есть заголовок!
		_current_packet_header = epp_header;
	}

	// Раз у нас есть заголовок - то может у нас хватает байтиков и на сам пакет?
	const auto current_packet_size = _current_packet_header->real_packet_size();
	if (current_packet_size > _accumulator.size())
		return; // Нет, не хватает

	auto current_packet_end = std::next(_accumulator.cbegin(), current_packet_size);
	// Чудесно! Только секундочку... А это не idle пакет?
	// Такие мы пользователю давать не будем
	if (!_emit_idle_packets && static_cast<int>(epp::protocol_id_t::IDLE) == _current_packet_header->protocol_id)
		_drop_accum(current_packet_end);
	else
		_flush_accum(current_packet_end, 0);

	// Выгребаем дальше
	goto again;
}


void map_packet_acceptor::_flush_accum(int event_flags)
{
	_flush_accum(_accumulator.end(), event_flags);
}


void map_packet_acceptor::_flush_accum(accum_t::const_iterator flush_zone_end, int event_flags)
{
	if (!_accumulator.size())
		return;

	acceptor_event_map_sdu event;
	event.channel_id = this->channel_id;
	event.qos = _prev_frame_qos;
	event.flags = event_flags | acceptor_event_map_sdu::MAPP;

	event.data.reserve(std::distance(_accumulator.cbegin(), flush_zone_end));
	std::copy(_accumulator.cbegin(), flush_zone_end, std::back_inserter(event.data));
	_drop_accum(flush_zone_end);

	emit_event(event);
}


void map_packet_acceptor::_drop_accum(accum_t::const_iterator drop_zone_end)
{
	_accumulator.erase(_accumulator.begin(), drop_zone_end);
	_current_packet_header.reset();
}


void map_packet_acceptor::_process_stay(const uint8_t * begin, const uint8_t * end)
{
	if (!_emit_stray_data)
		return;

	if (begin == end)
		return;

	// Если настроен сброс мусорных данных - собственно их и сбросим
	acceptor_event_map_sdu event;
	event.channel_id = this->channel_id;
	event.qos = _prev_frame_qos;
	event.flags = acceptor_event_map_sdu::MAPP | acceptor_event_map_sdu::STRAY;
	std::copy(begin, end, std::back_inserter(event.data));
	emit_event(event);
}


}}
