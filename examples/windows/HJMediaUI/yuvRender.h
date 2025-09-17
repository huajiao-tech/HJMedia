#include "HJPrerequisites.h"
#include <glad/gl.h> //#define GLAD_GL_IMPLEMENTATION  only implementation once;


NS_HJ_BEGIN

class HJYuvReader;
class HJSPBuffer;
class yuvRender
{
public:
    HJ_DEFINE_CREATE(yuvRender)
    yuvRender();    
    virtual ~yuvRender();
    int init(const std::string& i_url, int i_width, int i_height);
    void draw();
    
private:
    bool m_bCreateTexture = false;
    GLuint m_textureId = 0;
    std::shared_ptr<HJYuvReader> m_yuvReader = nullptr;
    std::shared_ptr<HJSPBuffer> m_RGBBuffer = nullptr;
};


NS_HJ_END