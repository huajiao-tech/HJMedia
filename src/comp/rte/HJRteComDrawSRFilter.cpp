#include "HJRteComDraw.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>

#include "HJFLog.h"
#include "HJOGBaseShader.h"
#include "HJOGCommon.h"
#include "HJOGFBOCtrl.h"

NS_HJ_BEGIN

namespace {

constexpr int HJRteSrScaleFactor = 2;
constexpr int HJRteSrMatchLineWidth = 2;
constexpr float HJRteSrMaxRcasStops = 2.0f;
constexpr float HJRteSrExtraSharpenScale = 1.65f;
constexpr float HJRteSrExtraSharpenEdgeStart = 0.025f;
constexpr float HJRteSrExtraSharpenEdgeEnd = 0.09f;

uint32_t HJRteSrFloatAsUint(float i_value)
{
    uint32_t bits = 0;
    std::memcpy(&bits, &i_value, sizeof(bits));
    return bits;
}

class HJOGShaderFilterSREasu : public HJOGCopyShaderStrip
{
public:
    HJ_DEFINE_CREATE(HJOGShaderFilterSREasu);

    HJOGShaderFilterSREasu()
    {
        HJ_SetInsName(HJOGShaderFilterSREasu);
    }

    void setConstants(const std::array<uint32_t, 4>& i_con0, const std::array<uint32_t, 4>& i_con1,
        const std::array<uint32_t, 4>& i_con2, const std::array<uint32_t, 4>& i_con3)
    {
        m_con0 = i_con0;
        m_con1 = i_con1;
        m_con2 = i_con2;
        m_con3 = i_con3;
    }

    virtual int shaderGetHandle(GLuint i_program) override
    {
        m_con0Handle = glGetUniformLocation(i_program, "uConst0");
        m_con1Handle = glGetUniformLocation(i_program, "uConst1");
        m_con2Handle = glGetUniformLocation(i_program, "uConst2");
        m_con3Handle = glGetUniformLocation(i_program, "uConst3");
        return (m_con0Handle == -1 || m_con1Handle == -1 || m_con2Handle == -1 || m_con3Handle == -1) ? HJErrFatal : HJ_OK;
    }

    virtual void shaderDrawUpdate() override
    {
        glUniform4ui(m_con0Handle, m_con0[0], m_con0[1], m_con0[2], m_con0[3]);
        glUniform4ui(m_con1Handle, m_con1[0], m_con1[1], m_con1[2], m_con1[3]);
        glUniform4ui(m_con2Handle, m_con2[0], m_con2[1], m_con2[2], m_con2[3]);
        glUniform4ui(m_con3Handle, m_con3[0], m_con3[1], m_con3[2], m_con3[3]);
    }

    virtual std::string shaderGetFragment() override
    {
        return std::string(HJOGCommon::HJ_GLSL_VERSION) +
            std::string(HJOGCommon::HJ_GLSL_PRECISION) +
            R"(
#ifdef GL_ES
precision highp float;
precision highp int;
#endif
in vec2 v_texcood;
uniform sampler2D sTexture;
uniform uvec4 uConst0;
uniform uvec4 uConst1;
uniform uvec4 uConst2;
uniform uvec4 uConst3;
out vec4 FragColor;

#define AP1 bool
#define AU1 uint
#define AU2 uvec2
#define AU3 uvec3
#define AU4 uvec4
#define AF1 float
#define AF2 vec2
#define AF3 vec3
#define AF4 vec4

AF1 AF1_x(AF1 a){return AF1(a);}
AF2 AF2_x(AF1 a){return AF2(a,a);}
AF3 AF3_x(AF1 a){return AF3(a,a,a);}
AF4 AF4_x(AF1 a){return AF4(a,a,a,a);}
#define AF1_(a) AF1_x(AF1(a))
#define AF2_(a) AF2_x(AF1(a))
#define AF3_(a) AF3_x(AF1(a))
#define AF4_(a) AF4_x(AF1(a))
AU1 AU1_x(AU1 a){return AU1(a);}
#define AU1_(a) AU1_x(AU1(a))
#define AF1_AU1(x) uintBitsToFloat(AU1(x))
#define AF2_AU2(x) uintBitsToFloat(AU2(x))
#define AF3_AU3(x) uintBitsToFloat(AU3(x))
#define AF4_AU4(x) uintBitsToFloat(AU4(x))
#define AU1_AF1(x) floatBitsToUint(AF1(x))

AF2 FsrEasuGatherCoord(AF2 p, AF2 texSize, AF2 texelOffset)
{
    AF2 base = floor(p * texSize - AF2_(0.5)) + AF2_(0.5);
    return (base + texelOffset) / texSize;
}

AF4 FsrEasuGather(AF2 p, int componentIndex)
{
    AF2 texSize = AF2(textureSize(sTexture, 0));
    AF4 sample0 = texture(sTexture, FsrEasuGatherCoord(p, texSize, AF2(0.0, 1.0)));
    AF4 sample1 = texture(sTexture, FsrEasuGatherCoord(p, texSize, AF2(1.0, 1.0)));
    AF4 sample2 = texture(sTexture, FsrEasuGatherCoord(p, texSize, AF2(1.0, 0.0)));
    AF4 sample3 = texture(sTexture, FsrEasuGatherCoord(p, texSize, AF2(0.0, 0.0)));

    if (componentIndex == 0)
    {
        return AF4(sample0.r, sample1.r, sample2.r, sample3.r);
    }
    if (componentIndex == 1)
    {
        return AF4(sample0.g, sample1.g, sample2.g, sample3.g);
    }
    return AF4(sample0.b, sample1.b, sample2.b, sample3.b);
}

AF4 FsrEasuRF(AF2 p){return FsrEasuGather(p, 0);}
AF4 FsrEasuGF(AF2 p){return FsrEasuGather(p, 1);}
AF4 FsrEasuBF(AF2 p){return FsrEasuGather(p, 2);}
AF1 APrxLoRcpF1(AF1 a){return AF1_AU1(AU1_(0x7ef07ebb)-AU1_AF1(a));}
AF1 ASatF1(AF1 x){return clamp(x, AF1_(0.0), AF1_(1.0));}
AF1 APrxLoRsqF1(AF1 a){return AF1_AU1(AU1_(0x5f347d74)-(AU1_AF1(a)>>AU1_(1)));}
AF3 AMin3F3(AF3 x,AF3 y,AF3 z){return min(x,min(y,z));}
AF3 AMax3F3(AF3 x,AF3 y,AF3 z){return max(x,max(y,z));}
AF1 ARcpF1(AF1 x){return AF1_(1.0)/x;}

void FsrEasuTapF(inout AF3 aC,inout AF1 aW,AF2 off,AF2 dir,AF2 len,AF1 lob,AF1 clp,AF3 c)
{
    AF2 v;
    v.x=(off.x*dir.x)+(off.y*dir.y);
    v.y=(off.x*(-dir.y))+(off.y*dir.x);
    v*=len;
    AF1 d2=v.x*v.x+v.y*v.y;
    d2=min(d2,clp);
    AF1 wB=AF1_(2.0/5.0)*d2+AF1_(-1.0);
    AF1 wA=lob*d2+AF1_(-1.0);
    wB*=wB;
    wA*=wA;
    wB=AF1_(25.0/16.0)*wB+AF1_(-(25.0/16.0-1.0));
    AF1 w=wB*wA;
    aC+=c*w;
    aW+=w;
}

void FsrEasuSetF(inout AF2 dir,inout AF1 len,AF2 pp,AP1 biS,AP1 biT,AP1 biU,AP1 biV,AF1 lA,AF1 lB,AF1 lC,AF1 lD,AF1 lE)
{
    AF1 w = AF1_(0.0);
    if (biS) w=(AF1_(1.0)-pp.x)*(AF1_(1.0)-pp.y);
    if (biT) w=pp.x*(AF1_(1.0)-pp.y);
    if (biU) w=(AF1_(1.0)-pp.x)*pp.y;
    if (biV) w=pp.x*pp.y;
    AF1 dc=lD-lC;
    AF1 cb=lC-lB;
    AF1 lenX=max(abs(dc),abs(cb));
    lenX=APrxLoRcpF1(lenX);
    AF1 dirX=lD-lB;
    dir.x+=dirX*w;
    lenX=ASatF1(abs(dirX)*lenX);
    lenX*=lenX;
    len+=lenX*w;
    AF1 ec=lE-lC;
    AF1 ca=lC-lA;
    AF1 lenY=max(abs(ec),abs(ca));
    lenY=APrxLoRcpF1(lenY);
    AF1 dirY=lE-lA;
    dir.y+=dirY*w;
    lenY=ASatF1(abs(dirY)*lenY);
    lenY*=lenY;
    len+=lenY*w;
}

void FsrEasuF(out AF3 pix, AU2 ip, AU4 con0, AU4 con1, AU4 con2, AU4 con3)
{
    AF2 pp=AF2(ip)*AF2_AU2(con0.xy)+AF2_AU2(con0.zw);
    AF2 fp=floor(pp);
    pp-=fp;
    AF2 p0=fp*AF2_AU2(con1.xy)+AF2_AU2(con1.zw);
    AF2 p1=p0+AF2_AU2(con2.xy);
    AF2 p2=p0+AF2_AU2(con2.zw);
    AF2 p3=p0+AF2_AU2(con3.xy);
    AF4 bczzR=FsrEasuRF(p0);
    AF4 bczzG=FsrEasuGF(p0);
    AF4 bczzB=FsrEasuBF(p0);
    AF4 ijfeR=FsrEasuRF(p1);
    AF4 ijfeG=FsrEasuGF(p1);
    AF4 ijfeB=FsrEasuBF(p1);
    AF4 klhgR=FsrEasuRF(p2);
    AF4 klhgG=FsrEasuGF(p2);
    AF4 klhgB=FsrEasuBF(p2);
    AF4 zzonR=FsrEasuRF(p3);
    AF4 zzonG=FsrEasuGF(p3);
    AF4 zzonB=FsrEasuBF(p3);
    AF4 bczzL=bczzB*AF4_(0.5)+(bczzR*AF4_(0.5)+bczzG);
    AF4 ijfeL=ijfeB*AF4_(0.5)+(ijfeR*AF4_(0.5)+ijfeG);
    AF4 klhgL=klhgB*AF4_(0.5)+(klhgR*AF4_(0.5)+klhgG);
    AF4 zzonL=zzonB*AF4_(0.5)+(zzonR*AF4_(0.5)+zzonG);
    AF1 bL=bczzL.x; AF1 cL=bczzL.y; AF1 iL=ijfeL.x; AF1 jL=ijfeL.y;
    AF1 fL=ijfeL.z; AF1 eL=ijfeL.w; AF1 kL=klhgL.x; AF1 lL=klhgL.y;
    AF1 hL=klhgL.z; AF1 gL=klhgL.w; AF1 oL=zzonL.z; AF1 nL=zzonL.w;
    AF2 dir=AF2_(0.0);
    AF1 len=AF1_(0.0);
    FsrEasuSetF(dir,len,pp,true,false,false,false,bL,eL,fL,gL,jL);
    FsrEasuSetF(dir,len,pp,false,true,false,false,cL,fL,gL,hL,kL);
    FsrEasuSetF(dir,len,pp,false,false,true,false,fL,iL,jL,kL,nL);
    FsrEasuSetF(dir,len,pp,false,false,false,true,gL,jL,kL,lL,oL);
    AF2 dir2=dir*dir;
    AF1 dirR=dir2.x+dir2.y;
    AP1 zro=dirR<AF1_(1.0/32768.0);
    dirR=APrxLoRsqF1(dirR);
    dirR=zro?AF1_(1.0):dirR;
    dir.x=zro?AF1_(1.0):dir.x;
    dir*=AF2_(dirR);
    len=len*AF1_(0.5);
    len*=len;
    AF1 stretch=(dir.x*dir.x+dir.y*dir.y)*APrxLoRcpF1(max(abs(dir.x),abs(dir.y)));
    AF2 len2=AF2(AF1_(1.0)+(stretch-AF1_(1.0))*len,AF1_(1.0)+AF1_(-0.5)*len);
    AF1 lob=AF1_(0.5)+AF1_((1.0/4.0-0.04)-0.5)*len;
    AF1 clp=APrxLoRcpF1(lob);
    AF3 min4=min(AMin3F3(AF3(ijfeR.z,ijfeG.z,ijfeB.z),AF3(klhgR.w,klhgG.w,klhgB.w),AF3(ijfeR.y,ijfeG.y,ijfeB.y)), AF3(klhgR.x,klhgG.x,klhgB.x));
    AF3 max4=max(AMax3F3(AF3(ijfeR.z,ijfeG.z,ijfeB.z),AF3(klhgR.w,klhgG.w,klhgB.w),AF3(ijfeR.y,ijfeG.y,ijfeB.y)), AF3(klhgR.x,klhgG.x,klhgB.x));
    AF3 aC=AF3_(0.0);
    AF1 aW=AF1_(0.0);
    FsrEasuTapF(aC,aW,AF2( 0.0,-1.0)-pp,dir,len2,lob,clp,AF3(bczzR.x,bczzG.x,bczzB.x));
    FsrEasuTapF(aC,aW,AF2( 1.0,-1.0)-pp,dir,len2,lob,clp,AF3(bczzR.y,bczzG.y,bczzB.y));
    FsrEasuTapF(aC,aW,AF2(-1.0, 1.0)-pp,dir,len2,lob,clp,AF3(ijfeR.x,ijfeG.x,ijfeB.x));
    FsrEasuTapF(aC,aW,AF2( 0.0, 1.0)-pp,dir,len2,lob,clp,AF3(ijfeR.y,ijfeG.y,ijfeB.y));
    FsrEasuTapF(aC,aW,AF2( 0.0, 0.0)-pp,dir,len2,lob,clp,AF3(ijfeR.z,ijfeG.z,ijfeB.z));
    FsrEasuTapF(aC,aW,AF2(-1.0, 0.0)-pp,dir,len2,lob,clp,AF3(ijfeR.w,ijfeG.w,ijfeB.w));
    FsrEasuTapF(aC,aW,AF2( 1.0, 1.0)-pp,dir,len2,lob,clp,AF3(klhgR.x,klhgG.x,klhgB.x));
    FsrEasuTapF(aC,aW,AF2( 2.0, 1.0)-pp,dir,len2,lob,clp,AF3(klhgR.y,klhgG.y,klhgB.y));
    FsrEasuTapF(aC,aW,AF2( 2.0, 0.0)-pp,dir,len2,lob,clp,AF3(klhgR.z,klhgG.z,klhgB.z));
    FsrEasuTapF(aC,aW,AF2( 1.0, 0.0)-pp,dir,len2,lob,clp,AF3(klhgR.w,klhgG.w,klhgB.w));
    FsrEasuTapF(aC,aW,AF2( 1.0, 2.0)-pp,dir,len2,lob,clp,AF3(zzonR.z,zzonG.z,zzonB.z));
    FsrEasuTapF(aC,aW,AF2( 0.0, 2.0)-pp,dir,len2,lob,clp,AF3(zzonR.w,zzonG.w,zzonB.w));
    pix=min(max4,max(min4,aC*AF3_(ARcpF1(aW))));
}

void main()
{
    if (v_texcood.x < -1.0)
    {
        discard;
    }
    AF3 c = AF3_(0.0);
    FsrEasuF(c, AU2(gl_FragCoord.xy), uConst0, uConst1, uConst2, uConst3);
    FragColor = vec4(c, 1.0);
}
)";
    }

private:
    GLint m_con0Handle = -1;
    GLint m_con1Handle = -1;
    GLint m_con2Handle = -1;
    GLint m_con3Handle = -1;
    std::array<uint32_t, 4> m_con0 = {};
    std::array<uint32_t, 4> m_con1 = {};
    std::array<uint32_t, 4> m_con2 = {};
    std::array<uint32_t, 4> m_con3 = {};
};

class HJOGShaderFilterSRRcas : public HJOGCopyShaderStrip
{
public:
    HJ_DEFINE_CREATE(HJOGShaderFilterSRRcas);

    HJOGShaderFilterSRRcas()
    {
        HJ_SetInsName(HJOGShaderFilterSRRcas);
    }

    void setConstants(const std::array<uint32_t, 4>& i_con0)
    {
        m_con0 = i_con0;
    }

    virtual int shaderGetHandle(GLuint i_program) override
    {
        m_con0Handle = glGetUniformLocation(i_program, "uConst0");
        return m_con0Handle == -1 ? HJErrFatal : HJ_OK;
    }

    virtual void shaderDrawUpdate() override
    {
        glUniform4ui(m_con0Handle, m_con0[0], m_con0[1], m_con0[2], m_con0[3]);
    }

    virtual std::string shaderGetFragment() override
    {
        return std::string(HJOGCommon::HJ_GLSL_VERSION) +
            std::string(HJOGCommon::HJ_GLSL_PRECISION) +
            R"(
#ifdef GL_ES
precision highp float;
precision highp int;
#endif
uniform sampler2D sTexture;
uniform uvec4 uConst0;
in vec2 v_texcood;
out vec4 FragColor;

#define AU1 uint
#define AF1 float
#define AF3 vec3
#define AF4 vec4
#define FSR_RCAS_LIMIT (0.25-(1.0/16.0))
#define FSR_RCAS_DENOISE 1

AF1 AF1_x(AF1 a){return AF1(a);}
#define AF1_(a) AF1_x(AF1(a))
AU1 AU1_x(AU1 a){return AU1(a);}
#define AU1_(a) AU1_x(AU1(a))
#define AF1_AU1(x) uintBitsToFloat(AU1(x))
#define AU1_AF1(x) floatBitsToUint(AF1(x))

AF1 APrxMedRcpF1(AF1 a){AF1 b=AF1_AU1(AU1_(0x7ef19fff)-AU1_AF1(a));return b*(-b*a+AF1_(2.0));}
AF1 AMax3F1(AF1 x,AF1 y,AF1 z){return max(x,max(y,z));}
AF1 AMin3F1(AF1 x,AF1 y,AF1 z){return min(x,min(y,z));}
AF1 ARcpF1(AF1 x){return AF1_(1.0)/x;}
AF1 ASatF1(AF1 x){return clamp(x,AF1_(0.0),AF1_(1.0));}
AF4 FsrRcasLoadF(ivec2 p) { return texelFetch(sTexture, p, 0); }
void FsrRcasInputF(inout AF1 r,inout AF1 g,inout AF1 b) {}

void FsrRcasF(out AF1 pixR, out AF1 pixG, out AF1 pixB, ivec2 sp, AU1 con)
{
    AF3 b=FsrRcasLoadF(sp+ivec2( 0,-1)).rgb;
    AF3 d=FsrRcasLoadF(sp+ivec2(-1, 0)).rgb;
    AF3 e=FsrRcasLoadF(sp).rgb;
    AF3 f=FsrRcasLoadF(sp+ivec2( 1, 0)).rgb;
    AF3 h=FsrRcasLoadF(sp+ivec2( 0, 1)).rgb;
    AF1 bR=b.r; AF1 bG=b.g; AF1 bB=b.b;
    AF1 dR=d.r; AF1 dG=d.g; AF1 dB=d.b;
    AF1 eR=e.r; AF1 eG=e.g; AF1 eB=e.b;
    AF1 fR=f.r; AF1 fG=f.g; AF1 fB=f.b;
    AF1 hR=h.r; AF1 hG=h.g; AF1 hB=h.b;
    FsrRcasInputF(bR,bG,bB);
    FsrRcasInputF(dR,dG,dB);
    FsrRcasInputF(eR,eG,eB);
    FsrRcasInputF(fR,fG,fB);
    FsrRcasInputF(hR,hG,hB);
    AF1 bL=bB*AF1_(0.5)+(bR*AF1_(0.5)+bG);
    AF1 dL=dB*AF1_(0.5)+(dR*AF1_(0.5)+dG);
    AF1 eL=eB*AF1_(0.5)+(eR*AF1_(0.5)+eG);
    AF1 fL=fB*AF1_(0.5)+(fR*AF1_(0.5)+fG);
    AF1 hL=hB*AF1_(0.5)+(hR*AF1_(0.5)+hG);
    AF1 nz=AF1_(0.25)*bL+AF1_(0.25)*dL+AF1_(0.25)*fL+AF1_(0.25)*hL-eL;
    nz=ASatF1(abs(nz)*APrxMedRcpF1(AMax3F1(AMax3F1(bL,dL,eL),fL,hL)-AMin3F1(AMin3F1(bL,dL,eL),fL,hL)));
    nz=AF1_(-0.5)*nz+AF1_(1.0);
    AF1 mn4R=min(AMin3F1(bR,dR,fR),hR);
    AF1 mn4G=min(AMin3F1(bG,dG,fG),hG);
    AF1 mn4B=min(AMin3F1(bB,dB,fB),hB);
    AF1 mx4R=max(AMax3F1(bR,dR,fR),hR);
    AF1 mx4G=max(AMax3F1(bG,dG,fG),hG);
    AF1 mx4B=max(AMax3F1(bB,dB,fB),hB);
    vec2 peakC=vec2(1.0,-4.0);
    AF1 hitMinR=min(mn4R,eR)*ARcpF1(AF1_(4.0)*mx4R);
    AF1 hitMinG=min(mn4G,eG)*ARcpF1(AF1_(4.0)*mx4G);
    AF1 hitMinB=min(mn4B,eB)*ARcpF1(AF1_(4.0)*mx4B);
    AF1 hitMaxR=(peakC.x-max(mx4R,eR))*ARcpF1(AF1_(4.0)*mn4R+peakC.y);
    AF1 hitMaxG=(peakC.x-max(mx4G,eG))*ARcpF1(AF1_(4.0)*mn4G+peakC.y);
    AF1 hitMaxB=(peakC.x-max(mx4B,eB))*ARcpF1(AF1_(4.0)*mn4B+peakC.y);
    AF1 lobeR=max(-hitMinR,hitMaxR);
    AF1 lobeG=max(-hitMinG,hitMaxG);
    AF1 lobeB=max(-hitMinB,hitMaxB);
    AF1 lobe=max(AF1_(-FSR_RCAS_LIMIT),min(AMax3F1(lobeR,lobeG,lobeB),AF1_(0.0)))*AF1_AU1(con);
    #ifdef FSR_RCAS_DENOISE
    lobe*=nz;
    #endif
    AF1 rcpL=APrxMedRcpF1(AF1_(4.0)*lobe+AF1_(1.0));
    pixR=(lobe*bR+lobe*dR+lobe*hR+lobe*fR+eR)*rcpL;
    pixG=(lobe*bG+lobe*dG+lobe*hG+lobe*fG+eG)*rcpL;
    pixB=(lobe*bB+lobe*dB+lobe*hB+lobe*fB+eB)*rcpL;
}

void main()
{
    if (v_texcood.x < -1.0)
    {
        discard;
    }
    AF3 c;
    FsrRcasF(c.r, c.g, c.b, ivec2(gl_FragCoord.xy), uConst0.x);
    FragColor = vec4(c, 1.0);
}
)";
    }

private:
    GLint m_con0Handle = -1;
    std::array<uint32_t, 4> m_con0 = {};
};

class HJOGShaderFilterSRSharpen : public HJOGCopyShaderStrip
{
public:
    HJ_DEFINE_CREATE(HJOGShaderFilterSRSharpen);

    HJOGShaderFilterSRSharpen()
    {
        HJ_SetInsName(HJOGShaderFilterSRSharpen);
    }

    void setParameters(float i_textureWidth, float i_textureHeight, float i_strength, float i_edgeStart, float i_edgeEnd)
    {
        m_textureWidth = i_textureWidth;
        m_textureHeight = i_textureHeight;
        m_strength = i_strength;
        m_edgeStart = i_edgeStart;
        m_edgeEnd = i_edgeEnd;
    }

    virtual int shaderGetHandle(GLuint i_program) override
    {
        m_textureSizeHandle = glGetUniformLocation(i_program, "uTextureSize");
        m_strengthHandle = glGetUniformLocation(i_program, "uStrength");
        m_edgeStartHandle = glGetUniformLocation(i_program, "uEdgeStart");
        m_edgeEndHandle = glGetUniformLocation(i_program, "uEdgeEnd");
        return (m_textureSizeHandle == -1 || m_strengthHandle == -1 || m_edgeStartHandle == -1 || m_edgeEndHandle == -1) ? HJErrFatal : HJ_OK;
    }

    virtual void shaderDrawUpdate() override
    {
        const float texelWidth = m_textureWidth > 0.0f ? 1.0f / m_textureWidth : 0.0f;
        const float texelHeight = m_textureHeight > 0.0f ? 1.0f / m_textureHeight : 0.0f;
        glUniform4f(m_textureSizeHandle, m_textureWidth, m_textureHeight, texelWidth, texelHeight);
        glUniform1f(m_strengthHandle, m_strength);
        glUniform1f(m_edgeStartHandle, m_edgeStart);
        glUniform1f(m_edgeEndHandle, m_edgeEnd);
    }

    virtual std::string shaderGetFragment() override
    {
        return std::string(HJOGCommon::HJ_GLSL_VERSION) +
            std::string(HJOGCommon::HJ_GLSL_PRECISION) +
            R"(
#ifdef GL_ES
precision highp float;
precision highp int;
#endif
in vec2 v_texcood;
uniform sampler2D sTexture;
uniform vec4 uTextureSize;
uniform float uStrength;
uniform float uEdgeStart;
uniform float uEdgeEnd;
out vec4 FragColor;

float HJRteSrLuma(vec3 c)
{
    return dot(c, vec3(0.299, 0.587, 0.114));
}

void main()
{
    if (v_texcood.x < -1.0)
    {
        discard;
    }
    vec2 texel = uTextureSize.zw;

    vec3 c = texture(sTexture, v_texcood).rgb;
    vec3 n = texture(sTexture, v_texcood + vec2(0.0, -1.0) * texel).rgb;
    vec3 s = texture(sTexture, v_texcood + vec2(0.0,  1.0) * texel).rgb;
    vec3 w = texture(sTexture, v_texcood + vec2(-1.0, 0.0) * texel).rgb;
    vec3 e = texture(sTexture, v_texcood + vec2( 1.0, 0.0) * texel).rgb;
    vec3 nw = texture(sTexture, v_texcood + vec2(-1.0, -1.0) * texel).rgb;
    vec3 ne = texture(sTexture, v_texcood + vec2( 1.0, -1.0) * texel).rgb;
    vec3 sw = texture(sTexture, v_texcood + vec2(-1.0,  1.0) * texel).rgb;
    vec3 se = texture(sTexture, v_texcood + vec2( 1.0,  1.0) * texel).rgb;

    float lC = HJRteSrLuma(c);
    float lN = HJRteSrLuma(n);
    float lS = HJRteSrLuma(s);
    float lW = HJRteSrLuma(w);
    float lE = HJRteSrLuma(e);
    float lNW = HJRteSrLuma(nw);
    float lNE = HJRteSrLuma(ne);
    float lSW = HJRteSrLuma(sw);
    float lSE = HJRteSrLuma(se);
    float lBlur = (2.0 * (lN + lS + lW + lE) + lNW + lNE + lSW + lSE) / 12.0;
    float lDetail = clamp(lC - lBlur, -0.35, 0.35);

    float minL = min(lC, min(min(min(lN, lS), min(lW, lE)), min(min(lNW, lNE), min(lSW, lSE))));
    float maxL = max(lC, max(max(max(lN, lS), max(lW, lE)), max(max(lNW, lNE), max(lSW, lSE))));
    float edge = maxL - minL;
    float gate = smoothstep(uEdgeStart, uEdgeEnd, edge);

    float sharpenAmount = uStrength * (0.45 + 1.35 * gate);
    float sharpenedL = clamp(lC + lDetail * sharpenAmount, minL - edge * 0.18, maxL + edge * 0.18);
    float lumaScale = sharpenedL / max(lC, 0.05);
    vec3 minColor = min(c, min(min(n, s), min(w, e)));
    vec3 maxColor = max(c, max(max(n, s), max(w, e)));
    vec3 limit = vec3(edge * 0.18);
    vec3 sharpened = clamp(c * lumaScale, minColor - limit, maxColor + limit);
    sharpened = max(sharpened, vec3(0.0));

    FragColor = vec4(sharpened, 1.0);
}
)";
    }

private:
    GLint m_textureSizeHandle = -1;
    GLint m_strengthHandle = -1;
    GLint m_edgeStartHandle = -1;
    GLint m_edgeEndHandle = -1;
    float m_textureWidth = 0.0f;
    float m_textureHeight = 0.0f;
    float m_strength = 0.0f;
    float m_edgeStart = 0.0f;
    float m_edgeEnd = 0.0f;
};

} // namespace

HJRteComDrawSRFilter::HJRteComDrawSRFilter()
{
    HJ_SetInsName(HJRteComDrawSRFilter);
    setPriority(HJRteComPriority_VideoSR);
}

int HJRteComDrawSRFilter::init(HJBaseParam::Ptr i_param)
{
    int i_err = HJ_OK;
    do
    {
        i_err = HJRteComDrawFBO::init(i_param);
        if (i_err < 0)
        {
            break;
        }

        if (i_param)
        {
            HJ_CatchMapPlainGetVal(i_param, float, "sharpness", m_sharpness);
            HJ_CatchMapPlainGetVal(i_param, bool, "enableExtraSharpen", m_enableExtraSharpen);
            HJ_CatchMapPlainGetVal(i_param, float, "extraSharpenBoost", m_extraSharpenBoost);
            HJ_CatchMapPlainGetVal(i_param, float, "match", m_match);
            HJ_CatchMapPlainGetVal(i_param, int, "scaleFactor", m_scaleFactor);
            HJ_CatchMapPlainGetVal(i_param, bool, "dynamicScaleRatio", m_bDynamicScaleRatio);
        }

        m_scaleFactor = std::max(1, m_scaleFactor);
        HJFLogi("sr init effect param m_sharpness:{} enableExtraSharpen:{} extraSharpenBoost:{} match:{} scaleFactor:{} dynamicScaleRatio:{}",
            m_sharpness, m_enableExtraSharpen, m_extraSharpenBoost, m_match, m_scaleFactor, m_bDynamicScaleRatio);
        m_sharpness = priClamp01(m_sharpness);
        m_extraSharpenBoost = priClamp01(m_extraSharpenBoost);
        m_match = priClamp01(m_match);

        HJOGShaderFilterSREasu::Ptr easuShader = HJOGShaderFilterSREasu::Create();
        i_err = easuShader->init(OGCopyShaderStripFlag_2D, false);
        if (i_err < 0)
        {
            HJFLoge("{} init easu shader error:{}", getInsName(), i_err);
            break;
        }

        HJOGShaderFilterSRRcas::Ptr rcasShader = HJOGShaderFilterSRRcas::Create();
        i_err = rcasShader->init(OGCopyShaderStripFlag_2D, false);
        if (i_err < 0)
        {
            HJFLoge("{} init rcas shader error:{}", getInsName(), i_err);
            break;
        }

        HJOGShaderFilterSRSharpen::Ptr sharpenShader = HJOGShaderFilterSRSharpen::Create();
        i_err = sharpenShader->init(OGCopyShaderStripFlag_2D, false);
        if (i_err < 0)
        {
            HJFLoge("{} init sharpen shader error:{}", getInsName(), i_err);
            break;
        }

        m_easuShader = easuShader;
        m_rcasShader = rcasShader;
        m_sharpenShader = sharpenShader;
    } while (false);
    return i_err;
}

void HJRteComDrawSRFilter::done()
{
    m_easuFbo = nullptr;
    m_rcasFbo = nullptr;
    m_easuShader = nullptr;
    m_rcasShader = nullptr;
    m_sharpenShader = nullptr;
    HJRteComDrawFBO::done();
}

int HJRteComDrawSRFilter::setParam(HJBaseParam::Ptr i_param)
{
    if (!i_param)
    {
        return HJ_OK;
    }
    HJ_CatchMapPlainGetVal(i_param, float, "sharpness", m_sharpness);
    HJ_CatchMapPlainGetVal(i_param, bool, "enableExtraSharpen", m_enableExtraSharpen);
    HJ_CatchMapPlainGetVal(i_param, float, "extraSharpenBoost", m_extraSharpenBoost);
    HJ_CatchMapPlainGetVal(i_param, float, "match", m_match);
    HJ_CatchMapPlainGetVal(i_param, int, "scaleFactor", m_scaleFactor);
    HJ_CatchMapPlainGetVal(i_param, bool, "dynamicScaleRatio", m_bDynamicScaleRatio);
    m_scaleFactor = std::max(1, m_scaleFactor);
    m_sharpness = priClamp01(m_sharpness);
    m_extraSharpenBoost = priClamp01(m_extraSharpenBoost);
    m_match = priClamp01(m_match);

    HJFLogi("sr setParam effect param m_sharpness:{} enableExtraSharpen:{} extraSharpenBoost:{} match:{} scaleFactor:{} dynamicScaleRatio:{}",
        m_sharpness, m_enableExtraSharpen, m_extraSharpenBoost, m_match, m_scaleFactor, m_bDynamicScaleRatio);

    return HJ_OK;
}

void HJRteComDrawSRFilter::setScaleFactor(int i_scaleFactor)
{
    m_scaleFactor = std::max(1, i_scaleFactor);
}

void HJRteComDrawSRFilter::setDynamicScaleRatio(bool i_bDynamicScaleRatio)
{
    m_bDynamicScaleRatio = i_bDynamicScaleRatio;
}

int HJRteComDrawSRFilter::adjustResolution(int i_width, int i_height)
{
    m_inputWidth = std::max(0, i_width);
    m_inputHeight = std::max(0, i_height);
    return HJ_OK;
}

int HJRteComDrawSRFilter::bind()
{
    int i_err = HJ_OK;
    do
    {
        if (m_inputWidth <= 0 || m_inputHeight <= 0)
        {
            i_err = HJ_WOULD_BLOCK;
            HJFLogd("{} bind skip because input resolution is not ready: {}x{}", getInsName(), m_inputWidth, m_inputHeight);
            break;
        }

        bool bFromTarget = false;
        const HJSizei outputSize = priResolveOutputSizeFromTargets(m_inputWidth, m_inputHeight, bFromTarget);
        if (outputSize.w <= 0 || outputSize.h <= 0)
        {
            i_err = HJ_WOULD_BLOCK;
            HJFLoge("{} bind resolve output size invalid input:{}x{} output:{}x{}", getInsName(), m_inputWidth, m_inputHeight, outputSize.w, outputSize.h);
            break;
        }

        HJFLogi("{} bind resolve sr output {}x{} source:{} input:{}x{} scaleFactor:{} dynamicScaleRatio:{}",
            getInsName(),
            outputSize.w,
            outputSize.h,
            bFromTarget ? "target" : "fallback",
            m_inputWidth,
            m_inputHeight,
            priGetEffectiveScaleFactor(),
            m_bDynamicScaleRatio);

        i_err = HJRteComDrawFBO::adjustResolution(outputSize.w, outputSize.h);
        if (i_err < 0)
        {
            break;
        }
        i_err = HJRteComDrawFBO::bind();
        if (i_err < 0)
        {
            break;
        }
    } while (false);
    return i_err;
}

int HJRteComDrawSRFilter::draw(GLuint textureId, int i_fitMode, int srcw, int srch, int dstw, int dsth, float* texMat, bool i_bYFlip, bool i_bXFlip)
{
    HJ_UNUSED(i_fitMode);
    int i_err = HJ_OK;
    do
    {
        if (textureId == 0 || srcw <= 0 || srch <= 0)
        {
            i_err = HJErrInvalidParams;
            break;
        }
        if (!m_easuShader || !m_rcasShader)
        {
            i_err = HJErrFatal;
            break;
        }

        i_err = adjustResolution(srcw, srch);
        if (i_err < 0)
        {
            break;
        }

        int srDstWidth = std::max(0, dstw);
        int srDstHeight = std::max(0, dsth);
        if (srDstWidth <= 0 || srDstHeight <= 0)
        {
            bool bFromTarget = false;
            const HJSizei outputSize = priResolveOutputSizeFromTargets(srcw, srch, bFromTarget);
            srDstWidth = outputSize.w;
            srDstHeight = outputSize.h;
            if (srDstWidth <= 0 || srDstHeight <= 0)
            {
                i_err = HJErrInvalidParams;
                HJFLoge("{} draw resolve output size invalid input:{}x{} output:{}x{}", getInsName(), srcw, srch, srDstWidth, srDstHeight);
                break;
            }

            i_err = HJRteComDrawFBO::adjustResolution(srDstWidth, srDstHeight);
            if (i_err < 0)
            {
                break;
            }
        }
        if (srDstWidth != dstw || srDstHeight != dsth)
        {
            HJFLogi("{} draw sr output use bound size:{}x{} incoming:{}x{} input:{}x{}",
                getInsName(), srDstWidth, srDstHeight, dstw, dsth, srcw, srch);
        }

        const int srWidth = std::max(0, std::min(srDstWidth, static_cast<int>(std::lround(static_cast<double>(srDstWidth) * static_cast<double>(m_match)))));
        if (srWidth <= 0)
        {
            break;
        }
        const int srOffsetX = srDstWidth - srWidth;

        HJOGShaderFilterSREasu::Ptr easuShader = std::dynamic_pointer_cast<HJOGShaderFilterSREasu>(m_easuShader);
        HJOGShaderFilterSRRcas::Ptr rcasShader = std::dynamic_pointer_cast<HJOGShaderFilterSRRcas>(m_rcasShader);
        HJOGShaderFilterSRSharpen::Ptr sharpenShader = std::dynamic_pointer_cast<HJOGShaderFilterSRSharpen>(m_sharpenShader);
        if (!easuShader || !rcasShader)
        {
            i_err = HJErrFatal;
            break;
        }
        const float extraSharpness = priGetExtraSharpness();
        const bool useExtraSharpen = m_enableExtraSharpen && extraSharpness > 0.001f && sharpenShader;

        i_err = priEnsureIntermediateFbos(srDstWidth, srDstHeight);
        if (i_err < 0)
        {
            break;
        }

        std::array<uint32_t, 4> easuCon0 = {};
        std::array<uint32_t, 4> easuCon1 = {};
        std::array<uint32_t, 4> easuCon2 = {};
        std::array<uint32_t, 4> easuCon3 = {};
        std::array<uint32_t, 4> rcasCon = {};
        priBuildFsrOriginConstants(static_cast<unsigned int>(srcw), static_cast<unsigned int>(srch),
            static_cast<unsigned int>(srDstWidth), static_cast<unsigned int>(srDstHeight),
            easuCon0, easuCon1, easuCon2, easuCon3, rcasCon);

        if (!m_easuFbo || (useExtraSharpen && !m_rcasFbo))
        {
            i_err = HJErrFatal;
            break;
        }

        easuShader->setConstants(easuCon0, easuCon1, easuCon2, easuCon3);
        m_easuFbo->attach();
        glViewport(0, 0, srDstWidth, srDstHeight);
        i_err = easuShader->draw(textureId, HJWindowRenderMode_CLIP, srcw, srch, srDstWidth, srDstHeight, texMat, i_bYFlip, i_bXFlip);
        m_easuFbo->detach();
        if (i_err < 0)
        {
            break;
        }
        HJFPERLog5i("{} tryResolution SR srcwh:{}x{} dstwh{}x{} rcaswh:{}x{}", getInsName(), srcw, srch, srDstWidth, srDstHeight, m_rcasFbo->getWidth(), m_rcasFbo->getHeight());

        rcasShader->setConstants(rcasCon);
        if (useExtraSharpen)
        {
            m_rcasFbo->attach();
            glViewport(0, 0, srDstWidth, srDstHeight);
            i_err = rcasShader->draw(m_easuFbo->getTextureId(), HJWindowRenderMode_CLIP, srDstWidth, srDstHeight, srDstWidth, srDstHeight, nullptr, false, false);
            m_rcasFbo->detach();
            if (i_err < 0)
            {
                break;
            }

            sharpenShader->setParameters(static_cast<float>(srDstWidth), static_cast<float>(srDstHeight), extraSharpness,
                HJRteSrExtraSharpenEdgeStart, HJRteSrExtraSharpenEdgeEnd);
        }

        if (i_err < 0)
        {
            break;
        }

        const GLboolean wasScissorEnabled = glIsEnabled(GL_SCISSOR_TEST);
        GLint previousScissorBox[4] = { 0, 0, 0, 0 };
        GLfloat previousClearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        glGetIntegerv(GL_SCISSOR_BOX, previousScissorBox);
        glGetFloatv(GL_COLOR_CLEAR_VALUE, previousClearColor);
        glEnable(GL_SCISSOR_TEST);
        glScissor(srOffsetX, 0, srWidth, srDstHeight);

        if (useExtraSharpen)
        {
            glViewport(0, 0, srDstWidth, srDstHeight);
            i_err = sharpenShader->draw(m_rcasFbo->getTextureId(), HJWindowRenderMode_CLIP, srDstWidth, srDstHeight, srDstWidth, srDstHeight, nullptr, false, false);
        }
        else
        {
            glViewport(0, 0, srDstWidth, srDstHeight);
            i_err = rcasShader->draw(m_easuFbo->getTextureId(), HJWindowRenderMode_CLIP, srDstWidth, srDstHeight, srDstWidth, srDstHeight, nullptr, false, false);
        }

        if (i_err >= 0 && srWidth < srDstWidth)
        {
            const int lineX = std::max(0, std::min(srDstWidth - 1, srOffsetX - HJRteSrMatchLineWidth / 2));
            const int lineWidth = std::min(HJRteSrMatchLineWidth, srDstWidth - lineX);
            if (lineWidth > 0)
            {
                glScissor(lineX, 0, lineWidth, srDstHeight);
                glClearColor(0.0f, 1.0f, 0.0f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT);
            }
        }

        glClearColor(previousClearColor[0], previousClearColor[1], previousClearColor[2], previousClearColor[3]);
        glScissor(previousScissorBox[0], previousScissorBox[1], previousScissorBox[2], previousScissorBox[3]);
        if (!wasScissorEnabled)
        {
            glDisable(GL_SCISSOR_TEST);
        }
    } while (false);
    return i_err;
}

float HJRteComDrawSRFilter::priClamp01(float i_value) const
{
    return std::max(0.0f, std::min(1.0f, i_value));
}

float HJRteComDrawSRFilter::priGetExtraSharpness() const
{
    return priClamp01(m_extraSharpenBoost) * HJRteSrExtraSharpenScale;
}

int HJRteComDrawSRFilter::priGetEffectiveScaleFactor() const
{
    if (!m_bDynamicScaleRatio)
    {
        return std::max(1, m_scaleFactor);
    }

    const int minInputSize = std::min(m_inputWidth, m_inputHeight);
    if (minInputSize >= 1080)
    {
        return 1;
    }
    if (minInputSize >= 720)
    {
        return 2;
    }
    return 4;
}

HJSizei HJRteComDrawSRFilter::priGetFallbackOutputSize(int i_srcWidth, int i_srcHeight) const
{
    const int scaleFactor = priGetEffectiveScaleFactor();
    return HJSizei{
        std::max(1, i_srcWidth * scaleFactor),
        std::max(1, i_srcHeight * scaleFactor)
    };
}

HJSizei HJRteComDrawSRFilter::priComputeTargetOutputSize(int i_srcWidth, int i_srcHeight, int i_viewportWidth, int i_viewportHeight, int i_renderMode) const
{
    if (i_srcWidth <= 0 || i_srcHeight <= 0)
    {
        return HJSizei{ 0, 0 };
    }
    if (i_viewportWidth <= 0 || i_viewportHeight <= 0)
    {
        return HJSizei{ 0, 0 };
    }
    float scaleX = 0.0f;
    float scaleY = 0.0f;
    HJOGShaderCommon::GetScaleFromMode(i_renderMode, i_srcWidth, i_srcHeight, i_viewportWidth, i_viewportHeight, scaleX, scaleY);

    return HJSizei{
        std::max(1, static_cast<int>(std::lround(static_cast<double>(i_viewportWidth) * static_cast<double>(scaleX)))),
        std::max(1, static_cast<int>(std::lround(static_cast<double>(i_viewportHeight) * static_cast<double>(scaleY))))
    };
}

HJSizei HJRteComDrawSRFilter::priResolveOutputSizeFromTargets(int i_srcWidth, int i_srcHeight, bool& o_bFromTarget)
{
    o_bFromTarget = false;
    HJSizei bestSize = { 0, 0 };
    int64_t bestArea = -1;
    foreachNextLink([this, i_srcWidth, i_srcHeight, &o_bFromTarget, &bestSize, &bestArea](const std::shared_ptr<HJRteComLink>& i_link)
        {
            if (!i_link || !i_link->isDrawEnable())
            {
                return HJ_OK;
            }

            HJRteCom::Ptr dstCom = i_link->getDstComPtr();
            if (!dstCom || !dstCom->isTarget() || !dstCom->isEnable())
            {
                return HJ_OK;
            }

            HJRteComDraw::Ptr dstDraw = std::dynamic_pointer_cast<HJRteComDraw>(dstCom);
            if (!dstDraw)
            {
                return HJ_OK;
            }

            const HJSizei targetSize = dstDraw->getTargetWH();
            if (targetSize.w <= 0 || targetSize.h <= 0)
            {
                HJFLogd("{} skip unresolved target:{} size:{}x{}", getInsName(), dstCom->getInsName(), targetSize.w, targetSize.h);
                return HJ_OK;
            }

            const std::shared_ptr<HJRteComLinkInfo>& linkInfo = i_link->getLinkInfo();
            if (!linkInfo)
            {
                return HJ_OK;
            }

            const HJVec4i viewport = linkInfo->convert(targetSize.w, targetSize.h);
            if (viewport.z <= 0 || viewport.w <= 0)
            {
                HJFLogd("{} skip target:{} invalid viewport:{}x{}", getInsName(), dstCom->getInsName(), viewport.z, viewport.w);
                return HJ_OK;
            }

            const HJSizei candidateSize = priComputeTargetOutputSize(i_srcWidth, i_srcHeight, viewport.z, viewport.w, linkInfo->m_renderMode);
            if (candidateSize.w <= 0 || candidateSize.h <= 0)
            {
                return HJ_OK;
            }

            const int64_t candidateArea = static_cast<int64_t>(candidateSize.w) * static_cast<int64_t>(candidateSize.h);
            if (candidateArea > bestArea || (candidateArea == bestArea && candidateSize.w > bestSize.w) ||
                (candidateArea == bestArea && candidateSize.w == bestSize.w && candidateSize.h > bestSize.h))
            {
                bestArea = candidateArea;
                bestSize = candidateSize;
                o_bFromTarget = true;
                HJFPERLog5i("{} resolve sr target:{} canvas:{}x{} viewport:{}x{} renderMode:{} output:{}x{}",
                    getInsName(),
                    dstCom->getInsName(),
                    targetSize.w,
                    targetSize.h,
                    viewport.z,
                    viewport.w,
                    linkInfo->m_renderMode,
                    candidateSize.w,
                    candidateSize.h);
            }
            return HJ_OK;
        });

    if (!o_bFromTarget)
    {
        bestSize = priGetFallbackOutputSize(i_srcWidth, i_srcHeight);
        HJFLogi("{} resolve sr fallback output:{}x{} input:{}x{} scaleFactor:{} dynamicScaleRatio:{}",
            getInsName(), bestSize.w, bestSize.h, i_srcWidth, i_srcHeight, priGetEffectiveScaleFactor(), m_bDynamicScaleRatio);
    }
    return bestSize;
}

int HJRteComDrawSRFilter::priEnsureIntermediateFbos(int i_dstWidth, int i_dstHeight)
{
    int i_err = HJ_OK;
    do
    {
        if (!m_easuFbo || m_easuFbo->getWidth() != i_dstWidth || m_easuFbo->getHeight() != i_dstHeight)
        {
            HJFLogi("{} tryResolution recreate easu fbo old:{}x{} new:{}x{} dynamicScaleRatio:{} scaleFactor:{} input:{}x{}",
                getInsName(),
                m_easuFbo ? m_easuFbo->getWidth() : 0,
                m_easuFbo ? m_easuFbo->getHeight() : 0,
                i_dstWidth,
                i_dstHeight,
                m_bDynamicScaleRatio,
                priGetEffectiveScaleFactor(),
                m_inputWidth,
                m_inputHeight);
        }
        i_err = HJRteComDrawFBO::tryRestartFbo(m_easuFbo, i_dstWidth, i_dstHeight, true);
        if (i_err < 0)
        {
            break;
        }
        if (!m_rcasFbo || m_rcasFbo->getWidth() != i_dstWidth || m_rcasFbo->getHeight() != i_dstHeight)
        {
            HJFLogi("{} tryResolution recreate rcas fbo old:{}x{} new:{}x{} dynamicScaleRatio:{} scaleFactor:{} input:{}x{}",
                getInsName(),
                m_rcasFbo ? m_rcasFbo->getWidth() : 0,
                m_rcasFbo ? m_rcasFbo->getHeight() : 0,
                i_dstWidth,
                i_dstHeight,
                m_bDynamicScaleRatio,
                priGetEffectiveScaleFactor(),
                m_inputWidth,
                m_inputHeight);
        }
        i_err = HJRteComDrawFBO::tryRestartFbo(m_rcasFbo, i_dstWidth, i_dstHeight, true);
        if (i_err < 0)
        {
            break;
        }
    } while (false);
    return i_err;
}

void HJRteComDrawSRFilter::priBuildFsrOriginConstants(unsigned int i_inputWidth, unsigned int i_inputHeight,
    unsigned int i_outputWidth, unsigned int i_outputHeight, std::array<uint32_t, 4>& o_easuCon0,
    std::array<uint32_t, 4>& o_easuCon1, std::array<uint32_t, 4>& o_easuCon2, std::array<uint32_t, 4>& o_easuCon3,
    std::array<uint32_t, 4>& o_rcasCon) const
{
    const float inputWidth = static_cast<float>(i_inputWidth);
    const float inputHeight = static_cast<float>(i_inputHeight);
    const float outputWidth = static_cast<float>(i_outputWidth);
    const float outputHeight = static_cast<float>(i_outputHeight);

    o_easuCon0[0] = HJRteSrFloatAsUint(inputWidth / outputWidth);
    o_easuCon0[1] = HJRteSrFloatAsUint(inputHeight / outputHeight);
    o_easuCon0[2] = HJRteSrFloatAsUint(0.5f * inputWidth / outputWidth - 0.5f);
    o_easuCon0[3] = HJRteSrFloatAsUint(0.5f * inputHeight / outputHeight - 0.5f);

    o_easuCon1[0] = HJRteSrFloatAsUint(1.0f / inputWidth);
    o_easuCon1[1] = HJRteSrFloatAsUint(1.0f / inputHeight);
    o_easuCon1[2] = HJRteSrFloatAsUint(1.0f / inputWidth);
    o_easuCon1[3] = HJRteSrFloatAsUint(-1.0f / inputHeight);

    o_easuCon2[0] = HJRteSrFloatAsUint(-1.0f / inputWidth);
    o_easuCon2[1] = HJRteSrFloatAsUint(2.0f / inputHeight);
    o_easuCon2[2] = HJRteSrFloatAsUint(1.0f / inputWidth);
    o_easuCon2[3] = HJRteSrFloatAsUint(2.0f / inputHeight);

    o_easuCon3[0] = HJRteSrFloatAsUint(0.0f);
    o_easuCon3[1] = HJRteSrFloatAsUint(4.0f / inputHeight);
    o_easuCon3[2] = 0;
    o_easuCon3[3] = 0;

    const float sharpness = priClamp01(m_sharpness);
    const float rcasStops = (1.0f - sharpness) * HJRteSrMaxRcasStops;
    const float rcasAttenuation = std::exp2(-rcasStops);
    o_rcasCon[0] = HJRteSrFloatAsUint(rcasAttenuation);
    o_rcasCon[1] = 0;
    o_rcasCon[2] = 0;
    o_rcasCon[3] = 0;
}

NS_HJ_END
