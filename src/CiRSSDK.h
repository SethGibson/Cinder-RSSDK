#ifndef __CIRSSDK_H__
#define __CIRSSDK_H__
#ifdef _DEBUG
#pragma comment(lib, "libpxc_d.lib")
#else
#pragma comment(lib, "libpxc.lib")
#endif

#include <memory>
#include "pxcsensemanager.h"
#include "pxcprojection.h"
#include "cinder/Channel.h"
#include "cinder/gl/Texture.h"
#include "cinder/Surface.h"

using namespace ci;
using namespace std;

namespace CiRSSDK
{
	enum FrameSize
	{
		DEPTHSD,		//480x360
		DEPTHVGA,		//640(628)x480(468)
		DEPTHQVGA,		//320x240
		RGBVGA,			//640x480
		RGBHD			//1920x1080
	};

	enum AlignMode
	{
		ALIGN_FRAME,
		ALIGN_UVS_ONLY
	};

	class CinderRSSDK;
	typedef shared_ptr<CinderRSSDK> CinderRSRef;


	class CinderRSSDK
	{
	protected:
		CinderRSSDK();
	public:
		~CinderRSSDK();
		static CinderRSRef create() { return CinderRSRef(new CinderRSSDK()); }
		bool init();
		bool initRgb(const FrameSize& pSize, const float& pFPS);
		bool initDepth(const FrameSize& pSize, const float& pFPS, bool pAsColor);
		void enableAlignedImages(bool pState = true, AlignMode pMode = AlignMode::ALIGN_UVS_ONLY) { mShouldAlign = pState; mAlignMode = pMode; }

		bool start();
		bool update();
		bool stop();

		const Surface8u&	getRgbFrame();
		const Channel16u&	getDepthFrame();
		const Surface8u&	getDepth8uFrame();
		const Surface8u&	getColorMappedToDepthFrame();
		const Surface8u&	getDepthMappedToColorFrame();
		//Nomenclature Notes:
		//	"Space" denotes a 3d coordinate
		//	"Image" denotes an image space point ((0, width), (0,height), (image depth))
		//	"Coords" denotes texture space (U,V) coordinates
		//  "Frame" denotes a full Surface

		//get a camera space point from a depth image point
		const vec3		getDepthSpacePoint(float pImageX, float pImageY, float pImageZ);
		const vec3		getDepthSpacePoint(int pImageX, int pImageY, uint16_t pImageZ);
		const vec3		getDepthSpacePoint(vec3 pImageCoords);

		//get a Color object from a depth image point
		const Color		getColorFromDepthImage(float pImageX, float pImageY, float pImageZ);
		const Color		getColorFromDepthImage(int pImageX, int pImageY, uint16_t pImageZ);
		const Color		getColorFromDepthImage(vec3 pImageCoords);

		//get a Color object from a depth camera space point
		const Color		getColorFromDepthSpace(float pCameraX, float pCameraY, float pCameraZ);
		const Color		getColorFromDepthSpace(vec3 pCameraPoint);

		//get color space UVs from a depth image point
		const vec2		getColorCoordsFromDepthImage(float pImageX, float pImageY, float pImageZ);
		const vec2		getColorCoordsFromDepthImage(int pImageX, int pImageY, uint16_t pImageZ);
		const vec2		getColorCoordsFromDepthImage(vec3 pImageCoords);

		//get color space UVs from a depth space point
		const vec2		getColorCoordsFromDepthSpace(float pCameraX, float pCameraY, float pCameraZ);
		const vec2		getColorCoordsFromDepthSpace(vec3 pCameraPoint);

		const ivec2&	getDepthSize() { return mDepthSize;  }
		const int&		getDepthWidth() { return mDepthSize.x;  }
		const int&		getDepthHeight() { return mDepthSize.y; }

		const ivec2&	getRgbSize() { return mRgbSize; }
		const int&		getRgbWidth() { return mRgbSize.x; }
		const int&		getRgbHeight() { return mRgbSize.y; }

	private:
		bool			mIsInit,
						mIsRunning,
						mHasRgb,
						mHasDepth,
						mShouldAlign,
						mShouldGetDepthAsColor;

		AlignMode		mAlignMode;

		ivec2			mDepthSize;
		ivec2			mRgbSize;
		Surface8u		mRgbFrame;
		Surface8u		mDepth8uFrame;
		Surface8u		mColorToDepthFrame;
		Surface8u		mDepthToColorFrame;
		Channel16u		mDepthFrame;

		PXCSenseManager		*mSenseMgr;
		PXCProjection		*mCoordinateMapper;
		PXCCapture::Sample	*mCurrentSample;
	};


};
#endif