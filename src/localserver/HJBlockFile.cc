//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR: 
//CREATE TIME: 
//***********************************************************************************//
#include "HJBlockFile.h"
#include "HJFLog.h"
#include "HJException.h"
#include "HJXIOFile.h"
#include "HJFileUtil.h"

NS_HJ_BEGIN
//***********************************************************************************//
HJBlockFile::~HJBlockFile()
{
	done();
}

/**
 * @brief 
 * @param "*.blocks" 
 * @return int 
 */
int HJBlockFile::init(const std::string& file)
{
	m_file = file;
	try
	{ 
		if (!HJFileUtil::fileExist(m_file.c_str())) {
			return HJErrNotExist;
		}
		
		auto url = HJCreates<HJUrl>(file);
		auto xio = HJCreates<HJXIOFile>();
        int res = xio->open(url);
		if (HJ_OK != res) {
			return res;
		}
		auto size = xio->size();
		size_t predSize = sizeof(m_file_size) * 3 + sizeof(m_block_status) + sizeof(m_block_offsets);
		if (size != predSize) {
			return HJErrInvalidFile;
		}
		auto buffer = HJCreates<HJBuffer>(size);
		xio->read(buffer->data(), size);
		buffer->setSize(size);
		//
		buffer->read(reinterpret_cast<uint8_t *>(&m_file_size), sizeof(m_file_size));
		buffer->read(reinterpret_cast<uint8_t *>(&m_block_size), sizeof(m_block_size));
		buffer->read(reinterpret_cast<uint8_t *>(&m_total_blocks), sizeof(m_total_blocks));
		//
		for (size_t i = 0; i < m_total_blocks; i++) {
			bool status;
			buffer->read((uint8_t*)(&status), sizeof(bool));
			m_block_status[i] = status;
		}
		for (size_t i = 0; i < m_total_blocks; i++) {
			buffer->read(reinterpret_cast<uint8_t*>(&m_block_offsets[i]), sizeof(size_t));
		}
	} catch (const HJException& e) {
		HJFLoge("exception: {}", e.what());
		return HJErrInvalidPath;
	}
	return HJ_OK;
}

void HJBlockFile::done()
{

}

bool HJBlockFile::isFileComplete()
{
	for (size_t i = 0; i < m_total_blocks; i++) {
		if (!m_block_status[i]) {
			return false;
		}
	}
	return true;
}
void HJBlockFile::markBlockComplete(size_t block_index)
{
	if (block_index >= m_total_blocks) {
		return;
	}
	if(!m_block_status[block_index]) {
		m_block_status[block_index] = true;
		m_isDirty = true;
	}
}
std::vector<size_t> HJBlockFile::getIncompleteBlocks()
{
    std::vector<size_t> result;
	for (size_t i = 0; i < m_total_blocks; i++) {
		if (!m_block_status[i]) {
			result.push_back(i);
		}
	}
	return result;
}
std::pair<size_t, size_t> HJBlockFile::getBlockRange(size_t block_index)
{
	if (block_index >= m_total_blocks) {
		return { 0,0 };
	}
	size_t offset = m_block_offsets[block_index];
	size_t size = m_block_size;
	if (block_index == m_total_blocks - 1) {
		size = m_file_size - offset;
	}
	return { offset, size };
}
void HJBlockFile::flush()
{
	if (!m_isDirty) {
		return;
	}
	try
	{
		auto url = HJCreates<HJUrl>(m_file, HJ_XIO_WRITE);
		auto xio = HJCreates<HJXIOFile>();
		int res = xio->open(url);
		if (HJ_OK != res) {
			return;
		}
		auto buffer = HJCreates<HJBuffer>();
		//
		buffer->write(reinterpret_cast<uint8_t *>(&m_file_size), sizeof(m_file_size));
		buffer->write(reinterpret_cast<uint8_t *>(&m_block_size), sizeof(m_block_size));
		buffer->write(reinterpret_cast<uint8_t *>(&m_total_blocks), sizeof(m_total_blocks));
		//
		for (size_t i = 0; i < m_total_blocks; i++) {
			bool status = m_block_status[i];
			buffer->write((uint8_t*)(&status), sizeof(bool));
		}
		for (size_t i = 0; i < m_total_blocks; i++) {
			buffer->write(reinterpret_cast<uint8_t*>(&m_block_offsets[i]), sizeof(size_t));
		}
		//
		xio->write(buffer->data(), buffer->size());
		m_isDirty = false;
	} catch (const HJException& e) {
		HJFLoge("exception: {}", e.what());	
	}
	return;
}



NS_HJ_END