#include "HJSPBuffer.h"

NS_HJ_BEGIN

HJSPBuffer::~HJSPBuffer()
{
	priRelease();
}
HJSPBuffer::Ptr HJSPBuffer::create()
{
	std::shared_ptr<HJSPBuffer> ptr = std::make_shared<HJSPBuffer>();
	return ptr;
}
HJSPBuffer::Ptr HJSPBuffer::create(int i_nSize, unsigned char* i_pBuf)
{
	std::shared_ptr<HJSPBuffer> ptr = create();
	ptr->priCopy(i_nSize, i_pBuf);
	return ptr;
}
HJSPBuffer::Ptr HJSPBuffer::create(int i_nSize)
{
	std::shared_ptr<HJSPBuffer> ptr = create();
	ptr->priCreate(i_nSize);
	return ptr;
}


void HJSPBuffer::priRelease()
{
	if (m_pBuf)
	{
		delete[] m_pBuf;
		m_pBuf = NULL;
	}
	m_size = 0;
}
void HJSPBuffer::priCreate(int i_nSize)
{
	if (i_nSize > 0)
	{
		priRelease();
		m_size = i_nSize;
		m_pBuf = new unsigned char[m_size];
	}
}
void HJSPBuffer::priCopy(int i_nSize, unsigned char* i_pBuf)
{
	if (i_nSize > 0)
	{
		priRelease();
		m_size = i_nSize;
		m_pBuf = new unsigned char[m_size];
		memcpy(m_pBuf, i_pBuf, i_nSize);
	}
}

void HJSPBuffer::copy(int i_nSize, unsigned char* i_pBuf)
{
	priCopy(i_nSize, i_pBuf);
}

void HJSPBuffer::copy(const HJSPBuffer::Ptr& i_ptr)
{
	priCopy(i_ptr->m_size, i_ptr->m_pBuf);
}

bool HJSPBuffer::priIsMatch(int i_nSize, unsigned char* i_pBuf)
{
	bool bMatch = true;
	do
	{
		if (i_nSize <= 0)
		{
			bMatch = false;
			break;
		}
		if (m_size != i_nSize)
		{
			bMatch = false;
			break;
		}
		bMatch = (memcmp(m_pBuf, i_pBuf, m_size) == 0);
	} while (false);
	return bMatch;
}
bool HJSPBuffer::isMatch(int i_nSize, unsigned char* i_pBuf)
{
	return priIsMatch(i_nSize, i_pBuf);
}

bool HJSPBuffer::isMatch(const HJSPBuffer::Ptr& i_ptr)
{
	return priIsMatch(i_ptr->m_size, i_ptr->m_pBuf);
}

NS_HJ_END