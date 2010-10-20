/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009,2010  Alessandro Pignotti (a.pignotti@sssup.it)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

#ifndef RENDERING_H
#define RENDERING_H

#include "timer.h"
#include <FTGL/ftgl.h>

namespace lightspark
{

class RenderThread: public ITickJob
{
friend class DisplayObject;
private:
	SystemState* m_sys;
	pthread_t t;
	bool terminated;
	static void* sdl_worker(RenderThread*);
#ifdef COMPILE_PLUGIN
	NPAPI_params* npapi_params;
	static void* gtkplug_worker(RenderThread*);
#endif
	void commonGLInit(int width, int height);
	void commonGLResize(int width, int height);
	void commonGLDeinit();
	GLuint pixelBuffers[2];
	uint32_t currentPixelBuffer;
	uint32_t currentPixelBufferOffset;
	uint32_t pixelBufferWidth;
	uint32_t pixelBufferHeight;
	void resizePixelBuffers(uint32_t w, uint32_t h);
	ITextureUploadable* prevUploadJob;
	Mutex mutexLargeTexture;
	uint32_t largeTextureSize;
	struct LargeTexture
	{
		GLuint id;
		uint8_t* bitmap;
		LargeTexture(GLuint i, uint8_t* b):id(i),bitmap(b){}
		~LargeTexture(){/*delete[] bitmap;*/}
	};
	std::vector<LargeTexture> largeTextures;
	LargeTexture& allocateNewTexture();
	bool allocateChunkOnTextureCompact(LargeTexture& tex, TextureChunk& ret, uint32_t blocksW, uint32_t blocksH);
	bool allocateChunkOnTextureSparse(LargeTexture& tex, TextureChunk& ret, uint32_t blocksW, uint32_t blocksH);
	//Possible events to be handled
	//TODO: pad to avoid false sharing on the cache lines
	bool renderNeeded;
	bool uploadNeeded;
	sem_t event;
	sem_t inputDone;
	bool inputNeeded;
	bool inputDisabled;
	std::string fontPath;
	bool resizeNeeded;
	uint32_t newWidth;
	uint32_t newHeight;
	float scaleX;
	float scaleY;
	int offsetX;
	int offsetY;

#ifndef WIN32
	Display* mDisplay;
	GLXFBConfig mFBConfig;
	GLXContext mContext;
	Window mWindow;
#endif
	uint64_t time_s, time_d;

	bool loadShaderPrograms();
	uint32_t* interactive_buffer;
	bool tempBufferAcquired;
	void tick();
	int frameCount;
	int secsCount;
	Mutex mutexUploadJobs;
	std::deque<ITextureUploadable*> uploadJobs;
	/*
		Utility to get a job to do
	*/
	ITextureUploadable* getUploadJob();
	/*
		Common code to handle the core of the rendering
		@param testMode True if overlays can be displayed
	*/
	void coreRendering(FTFont& font, bool testMode);
	Semaphore initialized;
	class MaskData
	{
	public:
		DisplayObject* d;
		MATRIX m;
		MaskData(DisplayObject* _d, const MATRIX& _m):d(_d),m(_m){}
	};
	std::vector<MaskData> maskStack;
public:
	RenderThread(SystemState* s,ENGINE e, void* param=NULL);
	~RenderThread();
	void wait();
	void draw();
	float getIdAt(int x, int y);
	//The calling context MUST call this function with the transformation matrix ready
	void glAcquireTempBuffer(number_t xmin, number_t xmax, number_t ymin, number_t ymax);
	void glBlitTempBuffer(number_t xmin, number_t xmax, number_t ymin, number_t ymax);
	/**
		Allocates a chunk from the shared texture
	*/
	TextureChunk allocateTexture(uint32_t w, uint32_t h, bool compact);
	/**
		Release texture
	*/
	void releaseTexture(const TextureChunk& chunk);
	/**
		Render a quad of given size using the given chunk
	*/
	void renderTextured(const TextureChunk& chunk, uint32_t x, uint32_t y, uint32_t w, uint32_t h);
	/**
		Load the given data in the given texture chunk
	*/
	void loadChunkBGRA(const TextureChunk& chunk, uint32_t w, uint32_t h, uint8_t* data);
	/**
		Enqueue something to be uploaded to texture
	*/
	void addUploadJob(ITextureUploadable* u);
	/**
	  	Add a mask to the stack mask
		\pre A reference is not acquired, we assume the object life is protected until the corresponding pop
	*/
	void pushMask(DisplayObject* d, const MATRIX& m)
	{
		maskStack.emplace_back(d,m);
	}
	/**
	  	Remove the last pushed mask
	*/
	void popMask()
	{
		maskStack.pop_back();
	}
	bool isMaskPresent()
	{
		return !maskStack.empty();
	}
	void renderMaskToTmpBuffer() const;
	void requestInput();
	void requestResize(uint32_t w, uint32_t h);
	void waitForInitialization()
	{
		initialized.wait();
	}

	//OpenGL programs
	int gpu_program;
	int blitter_program;
	GLuint fboId;
	TextureBuffer dataTex;
	TextureBuffer tempTex;
	TextureBuffer inputTex;
	uint32_t windowWidth;
	uint32_t windowHeight;
	bool hasNPOTTextures;
	GLuint fragmentTexScaleUniform;
	
	InteractiveObject* selectedDebug;
};

};
#endif
