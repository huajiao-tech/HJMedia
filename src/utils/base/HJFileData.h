#pragma once

#include "HJPrerequisites.h"

NS_HJ_BEGIN

class HJDataBase
{
public:
	HJ_DEFINE_CREATE(HJDataBase);
	virtual ~HJDataBase();
	int open(const std::string& i_url, bool i_bRead);
protected:
	FILE* m_fp = NULL;
};


class HJDataReader : public HJDataBase
{
public:
	HJ_DEFINE_CREATE(HJDataReader);
	virtual ~HJDataReader();
	unsigned char* read(const std::string& i_url);
	int getLen()
	{
		return m_len;
	}
private:
	unsigned char* m_buf = NULL;
	int m_len = 0;
};


class HJYuvReader : public HJDataBase
{
public:
	HJ_DEFINE_CREATE(HJYuvReader);
	virtual ~HJYuvReader();
	int init(const std::string& i_url, int i_width, int i_height, bool i_bLoop = true);
	unsigned char* read();
	void done();
	int getWidth()
	{
		return m_width;
	}
	int getHeight()
	{
		return m_height;
	}
	int getYuvLen()
	{
		return m_yuvLen;
	}
	unsigned char* getY()
	{
		return m_pYuv;
	}
	unsigned char* getU()
	{
		return m_pYuv + m_width * m_height;
	}
	unsigned char* getV()
	{
		return m_pYuv + m_width * m_height * 5 / 4;
	}
private:
	int m_width  = 0;
	int m_height = 0;
	int m_yuvLen = 0;
	bool m_bLoop = true;
	unsigned char* m_pYuv = NULL;
};

class HJDataWriter : public HJDataBase
{
public:
	HJ_DEFINE_CREATE(HJDataWriter);
	virtual ~HJDataWriter();
	int init(const std::string& i_url);
	int write(unsigned char *i_pData, int i_nLen);
	int writeYuv(unsigned char** i_pData, int* i_nPitch, int i_width, int i_height);
};

class HJYuvWriter : public HJDataBase
{
public:
	HJ_DEFINE_CREATE(HJYuvWriter);
	virtual ~HJYuvWriter();
	int init(const std::string& i_url, int i_width, int i_height);
	int write(unsigned char** i_data, int* pitch);
	//int write(AVFrame* i_frame);
	int write(unsigned char* i_pYuv);
private:
	int priWrite(unsigned char *i_pdata, int width, int height, int pitch);
	int m_width = 0;
	int m_height = 0;
};

NS_HJ_END

