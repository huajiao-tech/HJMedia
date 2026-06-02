#pragma once

#include "HJPrerequisites.h"

typedef struct GLFWwindow GLFWwindow;

NS_HJ_BEGIN

class HJRenderCoreSingleton final
{
public:
    static HJRenderCoreSingleton& Instance();

    HJRenderCoreSingleton() = default;
    virtual ~HJRenderCoreSingleton() = default;

    int init();
    void done();
    GLFWwindow* getWindow() const;

    int makeContext();
    void makeCurrentNULL();

private:
    GLFWwindow* m_window = nullptr;
};

NS_HJ_END





