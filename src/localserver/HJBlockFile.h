//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR: 
//CREATE TIME: 
//***********************************************************************************//
#pragma once
#include "HJUtilitys.h"

NS_HJ_BEGIN
//***********************************************************************************//
class HJBlockFile : public HJObject
{
public:
	HJ_DECLARE_PUWTR(HJBlockFile);

	HJBlockFile(size_t file_size, size_t block_size) 
		: m_file_size(file_size)
		, m_block_size(block_size)
		, m_total_blocks((file_size + block_size - 1) / block_size)
		, m_block_status(m_total_blocks, false)
	{
		m_block_offsets.resize(m_total_blocks);
		for (size_t i = 0; i < m_total_blocks; ++i) {
			m_block_offsets[i] = i * m_block_size;
		}
	}
	virtual ~HJBlockFile() noexcept;

	int init(const std::string& file);
	void done();

	bool isFileComplete();
	void markBlockComplete(size_t block_index);
	std::vector<size_t> getIncompleteBlocks();
	std::pair<size_t, size_t> getBlockRange(size_t block_index);
	void flush();
private:
	std::string m_file{""};
	size_t m_file_size{ 0 };
	size_t m_block_size{0};
	size_t m_total_blocks{0};
	std::vector<bool> m_block_status;
	std::vector<size_t> m_block_offsets;
	bool m_isDirty{false};
};

NS_HJ_END