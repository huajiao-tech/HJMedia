package com.HJMediasdk.graphic.draw;

import android.content.Context;
import android.opengl.GLES20;
import android.opengl.GLES30;
import android.opengl.Matrix;


import com.HJMediasdk.utils.HJLog;
import com.HJMediasdk.utils.HJMyBase64;

import java.io.ByteArrayOutputStream;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;

import javax.crypto.Cipher;
import javax.crypto.CipherInputStream;
import javax.crypto.spec.IvParameterSpec;
import javax.crypto.spec.SecretKeySpec;

/**
 * Some OpenGL utility functions.
 */
public class HJGlUtil {
    public static final String TAG = "glutil";

    /** Identity matrix for general use.  Don't modify or life will get weird. */
    public static final float[] IDENTITY_MATRIX;
    static {
        IDENTITY_MATRIX = new float[16];
        Matrix.setIdentityM(IDENTITY_MATRIX, 0);
    }

    private static final int SIZEOF_FLOAT = 4;


    private HJGlUtil() {}     // do not instantiate

    public static int createProgram(Context applicationContext, int vertexSourceRawId,
                                    int fragmentSourceRawId) {

        String vertexSource = readTextFromRawResource(applicationContext, vertexSourceRawId);
        String fragmentSource = readTextFromRawResource(applicationContext, fragmentSourceRawId);

        return createProgram(vertexSource, fragmentSource);
    }

    /**
     * Creates a new program from the supplied vertex and fragment shaders.
     *
     * @return A handle to the program, or 0 on failure.
     */
    public static int createProgram(String vertexSource, String fragmentSource) {
    	int i_err = 0;
        int vertexShader = loadShader(GLES20.GL_VERTEX_SHADER, vertexSource);
        if (vertexShader == 0) {
            return 0;
        }
        int pixelShader = loadShader(GLES20.GL_FRAGMENT_SHADER, fragmentSource);
        if (pixelShader == 0) {
            return 0;
        }

        int program = GLES20.glCreateProgram();
        i_err = checkGlError("glCreateProgram");
        if (i_err < 0)
        {
        	program = -1;
        	return program;
        }
        if (program == 0) {
            HJLog.e(TAG, "Could not create program");
            return program;
        }
        GLES20.glAttachShader(program, vertexShader);
        i_err = checkGlError("glAttachShader");
        if (i_err < 0)
        {
        	return i_err;
        }
        GLES20.glAttachShader(program, pixelShader);
        i_err = checkGlError("glAttachShader");
        if (i_err < 0)
        {
        	return i_err;
        }
        GLES20.glLinkProgram(program);
        i_err = checkGlError("glLinkProgram");
        if (i_err < 0)
        {
        	return i_err;
        }
        int[] linkStatus = new int[1];
        GLES20.glGetProgramiv(program, GLES20.GL_LINK_STATUS, linkStatus, 0);
        if (linkStatus[0] != GLES20.GL_TRUE) {
            HJLog.e(TAG, "Could not link program: ");
            HJLog.e(TAG, GLES20.glGetProgramInfoLog(program));
            GLES20.glDeleteProgram(program);
            program = 0;
        }
        return program;
    }

    /**
     * Compiles the provided shader source.
     *
     * @return A handle to the shader, or 0 on failure.
     */
    public static int loadShader(int shaderType, String source) {
    	int i_err = 0;
        int shader = GLES20.glCreateShader(shaderType);
        i_err = checkGlError("glCreateShader type=" + shaderType);
        if (i_err < 0)
        {
        	return i_err;
        }
        GLES20.glShaderSource(shader, source);
        GLES20.glCompileShader(shader);
        int[] compiled = new int[1];
        GLES20.glGetShaderiv(shader, GLES20.GL_COMPILE_STATUS, compiled, 0);
        if (compiled[0] == 0) {
            HJLog.e(TAG, "Could not compile shader " + shaderType + ":");
            HJLog.e(TAG, " " + GLES20.glGetShaderInfoLog(shader));
            GLES20.glDeleteShader(shader);
            shader = 0;
        }
        return shader;
    }

    /**
     * Checks to see if a GLES error has been raised.
     */
    public static int checkGlError(String op) {
        int error = GLES20.glGetError();
        if (error != GLES20.GL_NO_ERROR) {
            String msg = op + ": glError 0x" + Integer.toHexString(error);
            HJLog.e(TAG, msg);
           // throw new RuntimeException(msg);
            return -1;
        }
        return 0;
    }

    /**
     * Checks to see if the location we obtained is valid.  GLES returns -1 if a label
     * could not be found, but does not set the GL error.
     * <p>
     * Throws a RuntimeException if the location is invalid.
     */
    public static void checkLocation(int location, String label) {
        if (location < 0) {
            throw new RuntimeException("Unable to locate '" + label + "' in program");
        }
    }

    /**
     * Creates a texture from raw data.
     *
     * @param data Image data, in a "direct" ByteBuffer.
     * @param width Texture width, in pixels (not bytes).
     * @param height Texture height, in pixels.
     * @param format Image data format (use constant appropriate for glTexImage2D(), e.g. GL_RGBA).
     * @return Handle to texture.
     */
    public static int createImageTexture(ByteBuffer data, int width, int height, int format) {
        int[] textureHandles = new int[1];
        int textureHandle;

        GLES20.glGenTextures(1, textureHandles, 0);
        textureHandle = textureHandles[0];
        HJGlUtil.checkGlError("glGenTextures");

        // Bind the texture handle to the 2D texture target.
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, textureHandle);

        // Configure min/mag filtering, i.e. what scaling method do we use if what we're rendering
        // is smaller or larger than the source image.
        GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MIN_FILTER,
                GLES20.GL_LINEAR);
        GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MAG_FILTER,
                GLES20.GL_LINEAR);
        HJGlUtil.checkGlError("loadImageTexture");

        // Load the data from the buffer into the texture handle.
        GLES20.glTexImage2D(GLES20.GL_TEXTURE_2D, /*level*/ 0, format,
                width, height, /*border*/ 0, format, GLES20.GL_UNSIGNED_BYTE, data);
        HJGlUtil.checkGlError("loadImageTexture");

        return textureHandle;
    }

    /**
     * Allocates a direct float buffer, and populates it with the float array data.
     */
    public static FloatBuffer createFloatBuffer(float[] coords) {
        // Allocate a direct ByteBuffer, using 4 bytes per float, and copy coords into it.
        ByteBuffer bb = ByteBuffer.allocateDirect(coords.length * SIZEOF_FLOAT);
        bb.order(ByteOrder.nativeOrder());
        FloatBuffer fb = bb.asFloatBuffer();
        fb.put(coords);
        fb.position(0);
        return fb;
    }

    /**
     * Writes GL version info to the HJLog.
     */
    public static void HJLogVersionInfo() {
        HJLog.i(TAG, "vendor  : " + GLES20.glGetString(GLES20.GL_VENDOR));
        HJLog.i(TAG, "renderer: " + GLES20.glGetString(GLES20.GL_RENDERER));
        HJLog.i(TAG, "version : " + GLES20.glGetString(GLES20.GL_VERSION));

        if (false) {
            int[] values = new int[1];
            GLES30.glGetIntegerv(GLES30.GL_MAJOR_VERSION, values, 0);
            int majorVersion = values[0];
            GLES30.glGetIntegerv(GLES30.GL_MINOR_VERSION, values, 0);
            int minorVersion = values[0];
            if (GLES30.glGetError() == GLES30.GL_NO_ERROR) {
                HJLog.i(TAG, "iversion: " + majorVersion + "." + minorVersion);
            }
        }
    }




    private static byte[] map = new byte[]{5, 19, 12, 6, 14, 2, 3, 0, 18, 10, 29, 11, 15, 9, 22, 28, 27, 21, 4, 7,
            30, 24, 23, 1, 25, 16, 8, 17, 31, 13, 20, 26};

    public static String[] splitString(String str) {
        // System.out.println(str);
        if (str.length() != 32) {
            return null;
        }
        // mix
        byte[] dst = new byte[32];
        byte[] scr = str.getBytes();
        for (int i = 0; i < 32; i++) {
            dst[i] = scr[map[i]];
        }
        // split
        String[] result = new String[2];
        result[0] = str.substring(5, 21);
        result[1] = str.substring(0, 5) + str.substring(21, 32);
        // System.out.println(result[0]);
        // System.out.println(result[1]);
        return result;
    }

    public static Cipher initAESCipher(String sKey, int cipherMode) {
//        Key gen
        Cipher cipher = null;
        try {
            sKey = HJMyBase64.encode(sKey.getBytes()).substring(0, 32);
            String[] data = splitString(sKey);
            SecretKeySpec skey = new SecretKeySpec(data[0].getBytes(), "AES");
            cipher = Cipher.getInstance("AES/CBC/PKCS5Padding");
            cipher.init(cipherMode, skey, new IvParameterSpec(data[1].getBytes()));
        } catch (Exception e) {
            e.printStackTrace();  //To change body of catch statement use File | Settings | File Templates.
        }
        return cipher;
    }

    public static String decryptFile(InputStream inputStream, String sKey) {
        try {
            Cipher cipher = initAESCipher(sKey, Cipher.DECRYPT_MODE);
            CipherInputStream cipherInputStream = new CipherInputStream(inputStream, cipher);
            ByteArrayOutputStream baos = new ByteArrayOutputStream();
            byte[] cache = new byte[1024];
            int nRead;
            while ((nRead = cipherInputStream.read(cache)) != -1) {
                baos.write(cache, 0, nRead);
            }
            cipherInputStream.close();
            return baos.toString();
        } catch (Exception e) {
            e.printStackTrace();  //To change body of catch statement use File | Settings | File Templates.
        } finally {
            try {
                inputStream.close();
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
        return null;
    }

    public static String readTextFromRawResource(final Context applicationContext, final int resourceId) {
        final InputStream inputStream =
                applicationContext.getResources().openRawResource(resourceId);
        String key = applicationContext.getResources().getResourceEntryName(resourceId) + ".glsl";
        return decryptFile(inputStream, key);
    }

}
