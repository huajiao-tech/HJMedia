package com.HJMediasdk.graphic.draw;

public interface HJShaderCb
{		
	public abstract String getVertexShader();
	public abstract String getFragmentShader(int shaderStyle);
	public abstract int    init_additional_parame(int programe);
	public abstract int    set_additional_parame();
	public abstract int    draw_end();
}