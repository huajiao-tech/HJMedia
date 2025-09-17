//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJFileDialogView.h"
#include "HJContext.h"
#include "HJFLog.h"
#include "HJImguiUtils.h"
#include "HJJson.h"
#include "ImFileDialog.h"

#define GL_CLAMP_TO_EDGE	0x812F
#define GL_BGRA				0x80E1

NS_HJ_BEGIN
//***********************************************************************************//
HJ_INSTANCE_IMP(HJFileDialogView);
//
const std::string HJFileDialogView::KEY_FOLDER = "";
const std::string HJFileDialogView::KEY_MEDIA_FILTER = "Media (*.mp4;*.flv;*.mov;*.mkv;*.ts){.mp4,.flv,.mov,.mkv,.ts},.*";;
const std::string HJFileDialogView::KEY_IMAGE_FILTER = "Image (*.png;*.jpg;*.jpeg;*.bmp;*.tga){.png,.jpg,.jpeg,.bmp,.tga},.*";
const std::string HJFileDialogView::KEY_MUSIC_FILTER = "Music (*.mp3;*.aac;*.m4a){.mp3,.aac,.m4a},.*";
const std::string HJFileDialogView::KEY_CC_FILTER = "CC FILE (*.c;*.cpp;*.cc;*.h;*.hpp){.c,.cpp,.cc,.h,.hpp},.*";
//
HJFileDialogView::HJFileDialogView()
{
	m_name = HJMakeGlobalName("FileDialog");
}

HJFileDialogView::~HJFileDialogView()
{
}

int HJFileDialogView::init(const std::string& title, const std::string& filter, bool isMultiselect, const std::string& startingDir)
{
	int res = HJ_OK;
	do
	{
		m_title = title;
		m_filter = filter;
		m_isMultiselect = isMultiselect;
		m_startingDir = startingDir;
		//
		ifd::FileDialog::Instance().Open(m_name, m_title, m_filter, isMultiselect, startingDir);
	} while (false);

	return res;
}

int HJFileDialogView::draw(const std::function<void(const std::vector<std::filesystem::path>&)>& cb)
{
	int res = HJ_OK;
	do
	{
		if (ifd::FileDialog::Instance().IsDone(m_name)) {
			if (ifd::FileDialog::Instance().HasResult()) {
				const std::vector<std::filesystem::path>& files = ifd::FileDialog::Instance().GetResults();
				if (cb) {
					cb(files);
				}
			}
			ifd::FileDialog::Instance().Close();
            m_isOpen = false;
		}
	} while (false);

	return res;
}

void HJFileDialogView::setGLReady()
{
	ifd::FileDialog::Instance().CreateTexture = [](uint8_t* data, int w, int h, char fmt) -> void* {
		GLuint tex;

		glGenTextures(1, &tex);
		glBindTexture(GL_TEXTURE_2D, tex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, (fmt == 0) ? GL_BGRA : GL_RGBA, GL_UNSIGNED_BYTE, data);
		//glGenerateMipmap(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0);

		return (void*)tex;
	};
	ifd::FileDialog::Instance().DeleteTexture = [](void* tex) {
		GLuint texID = (GLuint)((uintptr_t)tex);
		glDeleteTextures(1, &texID);
	};
}

NS_HJ_END
