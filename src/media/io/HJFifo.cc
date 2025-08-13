//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJFifo.h"
#include "HJFFUtils.h"
#include "HJFLog.h"

#define HJ_AUTO_GROW_DEFAULT_BYTES (1024 * 1024)

NS_HJ_BEGIN
//***********************************************************************************//
HJFifo::HJFifo(size_t elems, size_t size, unsigned int flags)
{
	if (!elems || !size) {
		return;
	}
	buffer = (uint8_t*)av_realloc_array(NULL, elems, elem_size);
	if (!buffer) {
		return;
	}
	nb_elems = elems;
	elem_size = size;
	is_empty = true;

	flags = flags;
	auto_grow_limit = HJ_MAX(HJ_AUTO_GROW_DEFAULT_BYTES / elem_size, 1);
}

HJFifo::~HJFifo()
{
	if (buffer) {
		av_freep(&buffer);
		buffer = NULL;
	}
}

int HJFifo::grow(size_t inc)
{
	if (inc > HJ_UINT64_MAX - nb_elems) {
		return HJErrMemAlloc;
	}
	uint8_t* tmp = (uint8_t *)av_realloc_array(buffer, nb_elems + inc, elem_size);
	if (!tmp) {
		return AVERROR(ENOMEM);
	}
	buffer = tmp;

	// move the data from the beginning of the ring buffer
    // to the newly allocated space
	if (offset_w <= offset_r && !is_empty) {
		const size_t copy = FFMIN(inc, offset_w);
		memcpy(tmp + nb_elems * elem_size, tmp, copy * elem_size);
		if (copy < offset_w) {
			memmove(tmp, tmp + copy * elem_size,
				(offset_w - copy) * elem_size);
			offset_w -= copy;
		} else {
			offset_w = copy == inc ? 0 : nb_elems + copy;
		}
	}
	nb_elems += inc;

	return HJ_OK;
}

int HJFifo::checkSpace(size_t to_write)
{
	const size_t can_write = canWriteSize();
	const size_t need_grow = to_write > can_write ? to_write - can_write : 0;
	size_t can_grow;

	if (!need_grow) {
		return 0;
	}

	can_grow = auto_grow_limit > nb_elems ?
			   auto_grow_limit - nb_elems : 0;
	if ((flags & HJ_FIFO_FLAG_AUTO_GROW) && need_grow <= can_grow) {
		// allocate a bit more than necessary, if we can
		const size_t inc = (need_grow < can_grow / 2) ? need_grow * 2 : can_grow;
		return grow(inc);
	}

	return HJErrENOSPC;
}

int HJFifo::commonWrite(const uint8_t* buf, size_t* nbelems, HJReadCB read_cb)
{
	size_t to_write = *nbelems;
	size_t ofw;
	int ret = 0;

	ret = checkSpace(to_write);
	if (ret < 0)
		return ret;

	ofw = offset_w;

	while (to_write > 0) {
		size_t    len = HJ_MIN(nb_elems - ofw, to_write);
		uint8_t* wptr = buffer + ofw * elem_size;

		if (read_cb) {
			ret = read_cb(wptr, &len);
			if (ret < 0 || len == 0)
				break;
		} else {
			memcpy(wptr, buf, len * elem_size);
			buf += len * elem_size;
		}
		ofw += len;
		if (ofw >= nb_elems)
			ofw = 0;
		to_write -= len;
	}
	offset_w = ofw;

	if (*nbelems != to_write)
		is_empty = false;
	*nbelems -= to_write;

	return ret;
}

int HJFifo::commonPeek(uint8_t* buf, size_t* nbelems, size_t offset, HJReadCB write_cb)
{
	size_t  to_read = *nbelems;
	size_t ofr = offset_r;
	size_t can_read = canReadSize();
	int         ret = 0;

	if (offset > can_read || to_read > can_read - offset) {
		*nbelems = 0;
		return HJErrEINVAL;
	}

	if (ofr >= nb_elems - offset)
		ofr -= nb_elems - offset;
	else
		ofr += offset;

	while (to_read > 0) {
		size_t    len = HJ_MIN(nb_elems - ofr, to_read);
		uint8_t* rptr = buffer + ofr * elem_size;

		if (write_cb) {
			ret = write_cb(rptr, &len);
			if (ret < 0 || len == 0)
				break;
		}
		else {
			memcpy(buf, rptr, len * elem_size);
			buf += len * elem_size;
		}
		ofr += len;
		if (ofr >= nb_elems)
			ofr = 0;
		to_read -= len;
	}

	*nbelems -= to_read;

	return ret;
}

void HJFifo::drain(size_t size) 
{
	const size_t cur_size = canReadSize();
	if (cur_size < size) {
		HJFLoge("error, drain cur_size:{}, size:{}", cur_size, size);
		return;
	}
	if (cur_size == size)
		is_empty = 1;

	if (offset_r >= nb_elems - size)
		offset_r -= nb_elems - size;
	else
		offset_r += size;
}

NS_HJ_END
