#include "VisualDataCamera.h"
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>
#include "HJFLog.h"

#ifdef _WIN32
#include <windows.h>
#include <dshow.h>
#include <comdef.h>
#pragma comment(lib, "strmiids.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#endif

using namespace std;
using namespace cv;

NS_HJ_BEGIN

#ifdef _WIN32
static string wideToConsole(const wstring& value)
{
	if (value.empty())
	{
		return string();
	}
	UINT codepage = GetConsoleOutputCP();
	if (codepage == 0)
	{
		codepage = CP_ACP;
	}
	int sizeNeeded = WideCharToMultiByte(codepage, 0, value.c_str(),
		static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
	string result(sizeNeeded, '\0');
	WideCharToMultiByte(codepage, 0, value.c_str(), static_cast<int>(value.size()),
		&result[0], sizeNeeded, nullptr, nullptr);
	return result;
}

static vector<pair<int, string>> listCameraNamesDShow()
{
	vector<pair<int, string>> cameras;
	HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	if (FAILED(hr) && hr != RPC_E_CHANGED_MODE)
	{
		return cameras;
	}

	ICreateDevEnum* devEnum = nullptr;
	hr = CoCreateInstance(CLSID_SystemDeviceEnum, nullptr, CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&devEnum));
	if (FAILED(hr) || !devEnum)
	{
		CoUninitialize();
		return cameras;
	}

	IEnumMoniker* enumMoniker = nullptr;
	hr = devEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &enumMoniker, 0);
	if (hr == S_OK && enumMoniker)
	{
		IMoniker* moniker = nullptr;
		ULONG fetched = 0;
		int index = 0;
		while (enumMoniker->Next(1, &moniker, &fetched) == S_OK)
		{
			IPropertyBag* propBag = nullptr;
			hr = moniker->BindToStorage(nullptr, nullptr, IID_PPV_ARGS(&propBag));
			if (SUCCEEDED(hr) && propBag)
			{
				VARIANT nameVar;
				VariantInit(&nameVar);
				hr = propBag->Read(L"FriendlyName", &nameVar, nullptr);
				if (SUCCEEDED(hr) && nameVar.vt == VT_BSTR)
				{
					wstring wideName(nameVar.bstrVal, SysStringLen(nameVar.bstrVal));
					cameras.push_back(make_pair(index, wideToConsole(wideName)));
				}
				VariantClear(&nameVar);
				propBag->Release();
			}
			moniker->Release();
			++index;
		}
		enumMoniker->Release();
	}

	devEnum->Release();
	CoUninitialize();
	return cameras;
}
#endif


VisualDataCamera::~VisualDataCamera()
{
	done();
}
int VisualDataCamera::init(HJBaseParam::Ptr i_param)
{
	int i_err = HJ_OK;
	do
	{
		i_err = VisualDataBase::init(i_param);
		if (i_err < 0)
		{
			break;
		}

		HJ_CatchMapPlainGetVal(i_param, int, VisualDataBase::s_paramWidth, m_width);
		HJ_CatchMapPlainGetVal(i_param, int, VisualDataBase::s_paramHeight, m_height);

		i_err = VisualDataBase::start();
		if (i_err < 0)
		{
			break;
		}
	} while (false);
	return i_err;
}
int VisualDataCamera::restart()
{
	int i_err = HJ_OK;
	do
	{
		vector<pair<int, string>> cameras = listCameraNamesDShow();
		if (!cameras.empty())
		{
			for (size_t i = 0; i < cameras.size(); ++i)
			{
				if (cameras[i].second == m_url)
				{
					m_cap = std::make_shared<cv::VideoCapture>(static_cast<int>(i), CAP_DSHOW);
					if (!m_cap->isOpened())
					{
						HJFLoge("open video failed {}", m_url);
						return HJErrInvalidParams;
					}

					if ((m_width > 0) && (m_height > 0))
					{
						m_cap->set(cv::CAP_PROP_FRAME_WIDTH, m_width);
						m_cap->set(cv::CAP_PROP_FRAME_HEIGHT, m_height);
					}
					break;
				}
			}
		}
		
		if (!m_cap)
		{
			HJFLoge("camera not found: {}", m_url);
			i_err = HJErrInvalidParams;
			break;
		}
	} while (false);
	return i_err;
}
std::shared_ptr<cv::Mat> VisualDataCamera::read()
{
	//HJFLogi("read enter");
	std::shared_ptr<cv::Mat> frame = std::make_shared<cv::Mat>();
	if (!m_cap->read(*frame))
	{
		m_cap = nullptr;
		return nullptr;
	}
	//HJFLogi("read end");
	return frame;
}

NS_HJ_END
