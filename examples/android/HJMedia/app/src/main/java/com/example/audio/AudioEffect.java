package com.example.audio;

public class AudioEffect {
	
//	static {
//        System.loadLibrary("effect_jni");
//    }
	
	public final static int MUSIC_HALL = 0;  //音乐厅
    public final static int SMALL_HALL = 1;  //小礼堂
    public final static int BIG_ROOM = 2;  //大房间
    public final static int SMALL_ROOM = 3;  //小房间
    public final static int ORIGINAL_SOUND = 4;
    
	public final static int V_ROOM_SIZE = 5;  
    public final static int V_WET = 6;  
    public final static int V_DRY = 7;  
    public final static int V_WIDTH = 8; 
    public final static int V_DAMP = 9;
    
    public static final String ROOM_SIZE = "roomsize";
    public static final String WET = "wet";
    public static final String DRY = "dry";
    public static final String WIDTH = "width";
    public static final String DAMP = "damp";
    
    @Override
    protected void finalize() throws Throwable {
        detachEffect();
        super.finalize();
    }
    
    /**
     * 连接开启音效
     */
    public void attachEffect()
    {

    }
    /**
     * 分离关闭音效
     */
    public void detachEffect()
    {

    }
    /**
     * 设置音效参数(0,1,2,3)对应音效如下(音乐厅,小礼堂,大房间,小房间)
     * @param param
     */
    public void setEffectParam(int param)
    {

    }
    
    /**
     * 设置单一音效参数
     * @param name: 参数名称("roomsize", "wet", "dry", "width", "damp")
     * @param param　(0~1.0)
     */
    public void setEffectParamByName(String name, float param)
    {

    }
    /**
     * 重置音效
     */
    public void reset()
    {

    }
    /**
     * 音效处理
     * @param dst: 音频输入和输出的数据容器
     * @param frames: 待处理数据的音频帧数
     * @param frame_size: 待处理数据的音频帧大小
     * @param channel: 待处理数据的音频声道
     * @return　处理的音频帧数
     */
    public int doEffect(byte[] dst, int frames, int frame_size, int channel)
    {
        return 0;
    }
    public int doEffect(short[] dst, int frames, int frame_size, int channel)
    {
        return 0;
    }
}
