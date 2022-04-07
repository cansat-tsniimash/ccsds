cmake_minimum_required(VERSION 3.16)


project(ccsds
	VERSION 0.1
	DESCRIPTION "Some CCSDS protocols implementation. Not complete and customized"
	LANGUAGES CXX
)


set(_INC_DIR include/ccsds)
set(_SRC_DIR src)


add_library(ccsds-common STATIC
	${_INC_DIR}/common/exceptions.hpp
	${_SRC_DIR}/common/exceptions.cpp
)


add_library(ccsds-epp STATIC
	${_INC_DIR}/epp/epp_header.hpp
	${_SRC_DIR}/epp/epp_header.cpp
)


add_library(ccsds-uslp STATIC
	${_INC_DIR}/uslp/events.hpp
	${_INC_DIR}/uslp/idle_pattern.hpp
	${_SRC_DIR}/uslp/idle_pattern.cpp
	${_INC_DIR}/uslp/input_stack.hpp
	${_SRC_DIR}/uslp/input_stack.cpp
	${_INC_DIR}/uslp/output_stack.hpp
	${_SRC_DIR}/uslp/output_stack.cpp

	${_INC_DIR}/uslp/common/defs.hpp
	${_INC_DIR}/uslp/common/frame_seq_no.hpp
	${_SRC_DIR}/uslp/common/frame_seq_no.cpp
	${_INC_DIR}/uslp/common/ids.hpp
	${_INC_DIR}/uslp/common/ids_io.hpp
	${_INC_DIR}/uslp/common/rr_muxer.hpp

	${_INC_DIR}/uslp/_detail/tfdf_header.hpp
	${_SRC_DIR}/uslp/_detail/tfdf_header.cpp
	${_INC_DIR}/uslp/_detail/tf_header.hpp
	${_SRC_DIR}/uslp/_detail/tf_header.cpp

	${_INC_DIR}/uslp/map/abstract_map.hpp
	${_SRC_DIR}/uslp/map/abstract_map.cpp
	${_INC_DIR}/uslp/map/map_access_acceptor.hpp
	${_SRC_DIR}/uslp/map/map_access_acceptor.cpp
	${_INC_DIR}/uslp/map/map_access_emitter.hpp
	${_SRC_DIR}/uslp/map/map_access_emitter.cpp
	${_INC_DIR}/uslp/map/map_packet_acceptor.hpp
	${_SRC_DIR}/uslp/map/map_packet_acceptor.cpp
	${_INC_DIR}/uslp/map/map_packet_emitter.hpp
	${_SRC_DIR}/uslp/map/map_packet_emitter.cpp

	${_INC_DIR}/uslp/master/abstract_mchannel.hpp
	${_SRC_DIR}/uslp/master/abstract_mchannel.cpp
	${_INC_DIR}/uslp/master/vchannel_demuxer.hpp
	${_SRC_DIR}/uslp/master/vchannel_demuxer.cpp
	${_INC_DIR}/uslp/master/vchannel_rr_muxer.hpp
	${_SRC_DIR}/uslp/master/vchannel_rr_muxer.cpp

	${_INC_DIR}/uslp/physical/abstract_pchannel.hpp
	${_SRC_DIR}/uslp/physical/abstract_pchannel.cpp
	${_INC_DIR}/uslp/physical/mchannel_demuxer.hpp
	${_SRC_DIR}/uslp/physical/mchannel_demuxer.cpp
	${_INC_DIR}/uslp/physical/mchannel_rr_muxer.hpp
	${_SRC_DIR}/uslp/physical/mchannel_rr_muxer.cpp

	${_INC_DIR}/uslp/virtual/abstract_vchannel.hpp
	${_SRC_DIR}/uslp/virtual/abstract_vchannel.cpp
	${_INC_DIR}/uslp/virtual/map_demuxer.hpp
	${_SRC_DIR}/uslp/virtual/map_demuxer.cpp
	${_INC_DIR}/uslp/virtual/map_rr_muxer.hpp
	${_SRC_DIR}/uslp/virtual/map_rr_muxer.cpp

)


target_link_libraries(ccsds-epp PUBLIC ccsds-common)
target_link_libraries(ccsds-uslp PUBLIC ccsds-common ccsds-epp)


target_include_directories(ccsds-common PUBLIC include)
target_include_directories(ccsds-epp PUBLIC include)
target_include_directories(ccsds-uslp PUBLIC include)


set_target_properties(ccsds-common
PROPERTIES
	CXX_STANDARD 17
	CXX_STANDARD_REQUIRED YES
	CXX_EXTENSIONS NO
)
set_target_properties(ccsds-epp
PROPERTIES
	CXX_STANDARD 17
	CXX_STANDARD_REQUIRED YES
	CXX_EXTENSIONS NO
)
set_target_properties(ccsds-uslp
PROPERTIES
	CXX_STANDARD 17
	CXX_STANDARD_REQUIRED YES
	CXX_EXTENSIONS NO
)


add_library(ccsds::common ALIAS ccsds-common)
add_library(ccsds::epp ALIAS ccsds-epp)
add_library(ccsds::uslp ALIAS ccsds-uslp)
