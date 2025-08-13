//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJUtilitys.h"

NS_HJ_BEGIN
using HJReadCB = std::function<int(uint8_t* buf, size_t* nb_elems)>;
//***********************************************************************************//
class HJFifo
{
public:
	using Ptr = std::shared_ptr<HJFifo>;
	HJFifo(size_t elems, size_t size, unsigned int flags);
	virtual ~HJFifo();

	//using HJFifoCB = std::function<int(void* opaque, void* buf, size_t* nb_elems)>;
	typedef enum Flags
	{
		HJ_FIFO_FLAG_NONE = 0,
		HJ_FIFO_FLAG_AUTO_GROW = (1 << 0)
	} Flags;

	void setAutoGrowLimit(const size_t maxElems) {
		auto_grow_limit = maxElems;
	}
	const size_t getElemSize() {
		return elem_size;
	}
	const size_t canReadSize() {
		if (offset_w <= offset_r && !is_empty)
			return nb_elems - offset_r + offset_w;
		return offset_w - offset_r;
	}
	const size_t canWriteSize() {
		return nb_elems - canReadSize();
	}

	int grow(size_t inc);
	int checkSpace(size_t to_write);
	int commonWrite(const uint8_t* buf, size_t* nbelems, HJReadCB read_cb);
	int write(const uint8_t* buf, size_t nbelems) {
		return commonWrite(buf, &nbelems, NULL);
	}
	int writeFromCB(HJReadCB read_cb, size_t* nbelems) {
		return commonWrite(NULL, nbelems, read_cb);
	}
	int commonPeek(uint8_t* buf, size_t* nbelems, size_t offset, HJReadCB write_cb);
	int read(uint8_t* buf, size_t nbelems) {
		int ret = commonPeek(buf, &nbelems, 0, NULL);
		drain(nbelems);
		return ret;
	}
	int readToCB(HJReadCB write_cb, size_t* nbelems) {
		int ret = commonPeek(NULL, nbelems, 0, write_cb);
		drain(*nbelems);
		return ret;
	}
	int peak(uint8_t* buf, size_t nbelems, size_t offset) {
		return commonPeek(buf, &nbelems, offset, NULL);
	}
	int peekToCB(HJReadCB write_cb, size_t* nbelems, size_t offset) {
		return commonPeek(NULL, nbelems, offset, write_cb);
	}
	void drain(size_t size);
	void reset() {
		offset_r = offset_w = 0;
		is_empty = true;
	}
private:
	uint8_t*		buffer = NULL;
	size_t			elem_size = 1;
	size_t			nb_elems = 0;
	size_t			offset_r = 0; 
	size_t			offset_w = 0;
	bool			is_empty = true;
	unsigned int	flags = HJ_FIFO_FLAG_NONE;
	size_t			auto_grow_limit = 0;
};

NS_HJ_END
