#include <ccsds/uslp/map/map_packet_emitter.hpp>

#include <cassert>
#include <cstring>
#include <sstream>

#include <ccsds/common/exceptions.hpp>

#include <ccsds/uslp/idle_pattern.hpp>
#include <ccsds/uslp/_detail/tfdf_header.hpp>
#include <ccsds/uslp/_detail/tf_header.hpp>


namespace ccsds { namespace uslp {


map_packet_emitter::map_packet_emitter(output_stack * stack, gmapid_t map_id_)
	: map_emitter(stack, map_id_)
{

}


void map_packet_emitter::push_sdu(
		payload_cookie_t cookie, const uint8_t * packet, size_t packet_size, qos_t qos
)
{
	data_unit_t du;

	// Проверим что это пакет, так как только их мы можем здесь отправлять
	const auto epp_header_size = epp::header_t::probe_header_size(*packet);
	if (0 == epp_header_size)
		throw einval_exception("supplied packet is not an epp packet. space packets is not supported here (yel?)");

	// Проверим размер пакета
	epp::header_t header;
	header.read(packet, packet_size);

	if (header.real_packet_size() != packet_size)
	{
		std::stringstream error;
		error << "invalid epp packet size in header. "
			<< "In header: " << header.real_packet_size() << ", "
			<< "provided: " << packet_size
		;
		throw einval_exception(error.str());
	}

	du.original_packet_size = packet_size;
	du.current_part_no = 0;
	du.qos = qos;
	du.cookie = cookie;

	du.packet.resize(packet_size);
	std::copy(packet, packet + packet_size, du.packet.begin());

	_data_queue.push_back(std::move(du));
}


void map_packet_emitter::finalize_impl()
{
	map_emitter::finalize_impl();

	// Убеждаемся что в tfdf влезает заголовок и хотябы один байтик информации
	if (detail::tfdf_header_t::full_size >= tfdf_size())
	{
		std::stringstream error;
		error << "tfdf zone is too small for map channel";
		throw einval_exception(error.str());
	}
}


bool map_packet_emitter::peek_tfdf_impl()
{
	return !_data_queue.empty();
}


bool map_packet_emitter::peek_tfdf_impl(output_map_frame_params & params)
{
	// У нас вообще что-нибудь есть там?
	if (_data_queue.empty())
		return false;

	params.payload_cookies.clear();

	// Окей, хоть один пакет у нас есть, значит собственно с qos следующего фрейма все понятно
	params.qos = _data_queue.front().qos;

	// Смотрим какой объем нам нужно занять
	const size_t tfdz_size = _tfdz_size();

	// Теперь смотрим то что мы должны отправить безразрывно - оно больше чем нужно набрать?
	// То есть - нужно ли нам блокировать на себя канал?

	// Для этого проходим по всем пакетам, что есть в буфере
	// И смотрим набирается ли нужное количество байт
	size_t continous_stream_size = 0;
	qos_t prev_qos = _data_queue.front().qos;
	for (auto itt = std::cbegin(_data_queue); itt != std::cend(_data_queue); itt++)
	{
		// Флаш и "отпускание канала" произойдет либо когда у нас кончатся данные. либо когда
		// У следующих пакетов сменится qos
		if (itt->qos != prev_qos)
			break;

		// Если мы видим что уже набрали данных больше чем на один фрейм -
		// продолжать нет смысла, останавливаем цикл
		if (continous_stream_size >= tfdz_size)
			break;

		// Окей, считаем этот пакет на отправку и продолжаем
		continous_stream_size += itt->packet.size();
		// Так же, если этот пакет целиком (или концом, что не суть важно) пойдет на отправку
		// - красим фрейм его кукой
		const bool is_final_part = continous_stream_size <= tfdz_size;
		payload_part_cookie_t ref{itt->cookie, itt->current_part_no, is_final_part};
		params.payload_cookies.push_back(std::move(ref));
	}

	// Мы посчитали сколько байт мы будем отправлять неразрывно
	// Теперь легко сказать заблокируем ли мы канал
	params.channel_lock = continous_stream_size > tfdz_size;
	return true;
}


void map_packet_emitter::pop_tfdf_impl(uint8_t * tfdf_buffer)
{
	// Если ничего нет - ничего и не дадим. Хотя по чесноку - этот метод не должны вызывать
	// если ничего нет
	if (_data_queue.empty())
		return;

	// Отступаем место под заголовок
	// У нас всегда полный размер заголовка
	uint8_t * const tfdz_start = tfdf_buffer + detail::tfdf_header_t::full_size;

	uint8_t * output_buffer = tfdz_start;
	uint16_t output_buffer_size = _tfdz_size();

	// Идем по пакетам и копируем их
	uint16_t first_valid_header_offset = 0xFFFF; // Специальное значение, показывающее NULL
	while (!_data_queue.empty())
	{
		auto & data_unit = _data_queue.front();

		// Определяем сколько мы можем скопировать
		const uint16_t to_copy_size = static_cast<uint16_t>(
			std::min(
					static_cast<size_t>(output_buffer_size),
					data_unit.packet.size()
			)
		);

		// Если мы сейчас пишем первый правильный заголовок - нужно бы это указать
		if (0xFFFF == first_valid_header_offset && data_unit.packet.size() == data_unit.original_packet_size)
			first_valid_header_offset = static_cast<uint16_t>(output_buffer - tfdz_start);

		// Копируем
		const auto to_copy_begin = std::cbegin(data_unit.packet);
		const auto to_copy_end = std::next(to_copy_begin, to_copy_size);
		std::copy(to_copy_begin, to_copy_end, output_buffer);
		data_unit.packet.erase(to_copy_begin, to_copy_end);
		// показываем что эту часть мы уже забрали
		data_unit.current_part_no++;

		// пересчитываем параметры выходного буфера
		output_buffer += to_copy_size;
		output_buffer_size -= to_copy_size;

		// если мы скопировали весь пакет - дропаем его
		if (data_unit.packet.empty())
			_data_queue.pop_front();

		// Если в выходном буфере не осталось места - останавливаемся
		if (0 == output_buffer_size)
			break;
	}

	// Если мы записали все что могли, а в выходном буфере все равно осталось место
	// Добиваем его idle пакетом
	if (output_buffer_size)
		_write_idle_packet(output_buffer, output_buffer_size);

	// Опять проверяем на первый валидный заголовок
	if (0xFFFF == first_valid_header_offset)
		first_valid_header_offset = static_cast<uint16_t>(output_buffer - tfdz_start);

	// Наконец-то пишем заголовок
	detail::tfdf_header_t header;
	header.ctr_rule = detail::tfdz_construction_rule_t::PACKETS_SPANNING_MULTIPLE_FRAMES;
	header.upid = detail::upid_t::CCSDS_PACKETS;
	header.first_header_offset = first_valid_header_offset;
	header.write(tfdf_buffer);
	// Готово!
}


uint16_t map_packet_emitter::_tfdz_size() const
{
	return map_emitter::tfdf_size() - detail::tfdf_header_t::full_size; // У нас всегда полный заголовок
}


void map_packet_emitter::_write_idle_packet(uint8_t * buffer, uint16_t buffer_size) const
{
	if (0 == buffer_size)
		return;

	epp::header_t header;
	header.protocol_id = static_cast<int>(epp::protocol_id_t::IDLE);
	header.real_packet_size(buffer_size);

	// Теперь пишем заголовок
	header.write(buffer, buffer_size);

	// Тело пакета заполним айдлами
	uint8_t * const pbody_buffer = buffer + header.size();
	const uint16_t pbody_size = buffer_size - header.size();
	const auto & idle_generator = idle_pattern_generator::instance();
	std::copy(
			idle_generator.begin(),
			idle_generator.end(pbody_size),
			pbody_buffer
	);
}

}}
