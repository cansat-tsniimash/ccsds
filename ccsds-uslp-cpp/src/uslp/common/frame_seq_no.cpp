#include <ccsds/uslp/common/frame_seq_no.hpp>

#include <sstream>

#include <ccsds/common/exceptions.hpp>


namespace ccsds { namespace uslp {


static const uint64_t _mask_for_byte_size(uint8_t mask_len)
{
	if (mask_len < 0 || mask_len > 8)
	{
		std::stringstream error;
		error << "unable to make bitmask for " << mask_len << " bytes";
		throw einval_exception(error.str());
	}

	static const uint64_t masks[9] = {
			0x0000000000000000, // 0
			0x00000000000000FF, // 1
			0x000000000000FFFF, // 2
			0x0000000000FFFFFF, // 3
			0x00000000FFFFFFFF, // 4
			0x000000FFFFFFFFFF, // 5
			0x0000FFFFFFFFFFFF, // 6
			0x00FFFFFFFFFFFFFF, // 7
			0xFFFFFFFFFFFFFFFF  // 8
	};

	return masks[mask_len];
}


const frame_seq_no_t & frame_seq_no_t::operator++()
{
	_value = (_value + 1) & _mask_for_byte_size(_value_size);
	return *this;
}


frame_seq_no_t frame_seq_no_t::operator++(int)
{
	frame_seq_no_t retval(*this);
	++(*this);
	return retval;
}


const frame_seq_no_t & frame_seq_no_t::operator--()
{
	_value = (_value - 1) & _mask_for_byte_size(_value_size);
	return *this;
}


frame_seq_no_t frame_seq_no_t::operator--(int)
{
	frame_seq_no_t retval(*this);
	--(*this);
	return retval;
}


frame_seq_no_t & frame_seq_no_t::operator+(uint64_t increment)
{
	_value = (_value + increment) & _mask_for_byte_size(_value_size);
	return *this;
}


frame_seq_no_t & frame_seq_no_t::operator-(uint64_t decrement)
{
	_value = (_value - decrement) & _mask_for_byte_size(_value_size);
	return *this;
}


const bool frame_seq_no_t::operator==(const frame_seq_no_t & other)
{
	return _value == other._value;
}


const bool frame_seq_no_t::operator!=(const frame_seq_no_t & other)
{
	return _value != other._value;
}


void frame_seq_no_t::value(uint64_t value_, uint8_t value_size_)
{
	// Сперва проверяем что нам дали правильный value_size
	if (value_size_ < 1 || value_size_ > 7)
	{
		std::stringstream error;
		error << "Invalid value of frame_seq_no_t value_size: " << static_cast<int>(value_size_) << ". "
				<< "Value size should be in range [1, 7]";
		throw einval_exception(error.str());
	}

	// Теперь проверяем что value_ в него влезает
	if (value_ >> 8*value_size_)
	{
		std::stringstream error;
		error << "Invalid frame_seq_no value: " << value_ << ". "
				<< "It will not fit value size, which is given as " << static_cast<int>(value_size_);
		throw einval_exception(error.str());
	}

	_value = value_;
	_value_size = value_size_;
}


}}
