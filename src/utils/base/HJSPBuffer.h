#pragma once

#include "HJPrerequisites.h"

NS_HJ_BEGIN

class HJSPBuffer
{
public:
	using Ptr = std::shared_ptr<HJSPBuffer>;

	static HJSPBuffer::Ptr create();
	static HJSPBuffer::Ptr create(int i_nSize, unsigned char* i_pBuf);
	static HJSPBuffer::Ptr create(int i_nSize);

	virtual ~HJSPBuffer();

	void copy(int i_nSize, unsigned char *i_pBuf);
	void copy(const HJSPBuffer::Ptr& i_ptr);

	bool isMatch(int i_nSize, unsigned char* i_pBuf);
	bool isMatch(const HJSPBuffer::Ptr& i_ptr);

	unsigned char *getBuf()
	{
		return m_pBuf;
	}
	int getSize()
	{
		return m_size;
	}

private:

	void priRelease();
	void priCopy(int i_nSize, unsigned char* i_pBuf);
	void priCreate(int i_nSize);
	bool priIsMatch(int i_nSize, unsigned char* i_pBuf);
	int m_size = 0;
	unsigned char* m_pBuf = NULL;
};

NS_HJ_END
 
