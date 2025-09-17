#include "HJFileData.h"
#include "HJFLog.h"

NS_HJ_BEGIN

HJDataBase::~HJDataBase()
{
	if (m_fp)
	{
		fclose(m_fp);
		m_fp = NULL;
	}
}
int HJDataBase::open(const std::string& i_url, bool i_bRead)
{
	int i_err = 0;
	do
	{
		std::string flag = "wb";
		if (i_bRead)
		{
			flag = "rb";
		}
		m_fp = fopen(i_url.c_str(), flag.c_str());
		if (!m_fp)
		{
			i_err = -1;
			HJFLoge("fopen error url:{}", i_url);
			break;
		}
	} while (false);
	return i_err;
}

////////////////////////////////////////////////////////////////////////////////////////
HJDataReader::~HJDataReader()
{
	if (m_buf)
	{
		delete[] m_buf;
		m_buf = nullptr;
	}
}
unsigned char* HJDataReader::read(const std::string& i_url)
{
	unsigned char* pBuf = NULL;
	do 
	{
		int i_err = HJDataBase::open(i_url, true);
		if (i_err < 0)
		{
			break;
		}
		fseek(m_fp, 0, SEEK_END);
		m_len = (int)ftell(m_fp);
		pBuf = new unsigned char[m_len];
		fseek(m_fp, 0, SEEK_SET);
		fread(pBuf, 1, m_len, m_fp);
		m_buf = pBuf;
	} while (0);
	return pBuf;	
}

////////////////////////////////////////////////////////////////////////////////////////
int HJYuvReader::init(const std::string& i_url, int i_width, int i_height, bool i_bLoop)
{
	int i_err = 0;
	do 
	{
		int i_err = HJDataBase::open(i_url, true);
		if (i_err < 0)
		{
			break;
		}
		m_width = i_width;
		m_height = i_height;
		m_bLoop = i_bLoop;

		HJFLogi("reader url:{} w:{} h:{} bloop:{} ", i_url, m_width, m_height, m_bLoop);

		if ((i_width <= 0) || (i_height <= 0))
		{
			i_err = -1;
			break;
		}

		m_yuvLen = m_width * m_height * 3 / 2;
		m_pYuv = new unsigned char[m_yuvLen];


	} while (false);
	return i_err;
}
unsigned char* HJYuvReader::read()
{
	unsigned char* pYuv = NULL;
	do
	{
		if (!m_fp)
		{
			break;
		}
		if ((m_yuvLen <= 0) || (m_pYuv == NULL))
		{
			break;
		}
		int len = (int)fread(m_pYuv, sizeof(unsigned char), (int)m_yuvLen, m_fp);
		if (len != m_yuvLen)
		{
			if (m_bLoop)
			{
				fseek(m_fp, 0, SEEK_SET);
				fread(m_pYuv, sizeof(unsigned char), m_yuvLen, m_fp);
				pYuv = m_pYuv;
			}
			else
			{
				pYuv = NULL;
			}
		}
		else
		{
			pYuv = m_pYuv;
		}
	} while (false);
	return pYuv;
}
HJYuvReader::~HJYuvReader()
{
	if (m_pYuv)
	{
		delete [] m_pYuv;
		m_pYuv = nullptr;
	}
}

//////////////////////////////////////////////////////
HJDataWriter::~HJDataWriter()
{
}
int HJDataWriter::init(const std::string& i_url)
{
	int i_err = 0;
	do 
	{
		int i_err = HJDataBase::open(i_url, false);
		if (i_err < 0)
		{
			break;
		}
	} while (false);
	return i_err;
}
int HJDataWriter::write(unsigned char* i_pData, int i_nLen)
{
	int i_err = 0;
	do 
	{
		if (!m_fp)
		{
			i_err = -1;
			break;
		}
		fwrite(i_pData, sizeof(unsigned char), i_nLen, m_fp);
	} while (0);
	return i_err;
}
int HJDataWriter::writeYuv(unsigned char** i_pData, int* i_nPitch, int i_width, int i_height)
{
	int i_err = 0;
	do 
	{
		for (int i = 0; i < 3; i++)
		{
			unsigned char* data = i_pData[i];
			int pitch = i_nPitch[i];
			int w = i_width;
			int h = i_height;
			if (i > 0)
			{
				w /= 2;
				h /= 2;
			}
			for (int j = 0; j < h; j++)
			{
				fwrite(data, sizeof(unsigned char), w, m_fp);
				data += pitch;
			}
		}
	} while (false);
	return i_err;
}
//////////////////////////////////////////////////////
SLYuvWriter::~SLYuvWriter()
{

}
int SLYuvWriter::init(const std::string& i_url, int i_width, int i_height)
{
	int i_err = 0;
	do 
	{
		int i_err = HJDataBase::open(i_url, false);
		if (i_err < 0)
		{
			break;
		}
		m_width = i_width;
		m_height = i_height;
	} while (0);
	return i_err;
}

int SLYuvWriter::priWrite(unsigned char* i_pdata, int width, int height, int pitch)
{
	int i_err = 0;
	do 
	{
		unsigned char* pData = i_pdata;
		for (int i = 0; i < height; i++)
		{
			fwrite(pData, 1, width, m_fp);
			pData += pitch;
		}
	} while (0);
	return i_err;
}
int SLYuvWriter::write(unsigned char* i_pYuv)
{
	int i_err = 0;
	do
	{
		if (!m_fp)
		{
			i_err = -1;
			break;
		}
		priWrite(i_pYuv, m_width, m_height, m_width);
		priWrite(i_pYuv + m_width * m_height, m_width / 2, m_height / 2, m_width / 2);
		priWrite(i_pYuv + m_width * m_height * 5 / 4, m_width / 2, m_height / 2, m_width / 2);
	} while (0);
	return i_err;
}
int SLYuvWriter::write(unsigned char** data, int* pitch)
{
	int i_err = 0;
	do 
	{
		if (!m_fp)
		{
			i_err = -1;
			break;
		}
		priWrite(data[0], m_width,     m_height,     pitch[0]);
		priWrite(data[1], m_width / 2, m_height / 2, pitch[1]);
		priWrite(data[2], m_width / 2, m_height / 2, pitch[2]);
	} while (0);
	return i_err;
}

#if 0
int SLYuvWriter::write(AVFrame* i_frame)
{
	unsigned char* data[3];
	int pitch[3];
	for (int i = 0; i < 3; i++)
	{
		data[i] = i_frame->data[i];
		pitch[i] = i_frame->linesize[i];
	}
	return write(data, pitch);	
}
#endif

NS_HJ_END
