#include "CiRSSDK.h"
#include "cinder/app/AppBase.h"

using namespace ci;
using namespace ci::app;

namespace CiRSSDK
{
	CinderRSSDK::CinderRSSDK() : mIsInit(false), mIsRunning(false), mHasRgb(false), mHasDepth(false), mShouldAlign(false), mShouldGetDepthAsColor(false)
	{
	}

	CinderRSSDK::~CinderRSSDK(){}
	bool CinderRSSDK::init()
	{
		mSenseMgr = PXCSenseManager::CreateInstance();
		if (mSenseMgr)
			mIsInit = true;

		return mIsInit;
	}

	bool CinderRSSDK::initRgb(const FrameSize& pSize, const float& pFPS)
	{
		pxcStatus cStatus;
		if (mSenseMgr)
		{
			switch (pSize)
			{
			case FrameSize::RGBVGA:
				mRgbSize = ivec2(640, 480);
				break;
			case FrameSize::RGBHD:
				mRgbSize = ivec2(1920, 1080);
				break;
			}
			cStatus = mSenseMgr->EnableStream(PXCCapture::STREAM_TYPE_COLOR, mRgbSize.x, mRgbSize.y, pFPS);
			if (cStatus >= PXC_STATUS_NO_ERROR)
			{
				mHasRgb = true;
				mRgbFrame = Surface8u(mRgbSize.x, mRgbSize.y, true, SurfaceChannelOrder::BGRA);
			}
		}

		return mHasRgb;
	}

	bool CinderRSSDK::initDepth(const FrameSize& pSize, const float& pFPS, bool pAsColor = false)
	{
		pxcStatus cStatus;
		if (mSenseMgr)
		{
			switch (pSize)
			{
			case FrameSize::DEPTHQVGA:
				mDepthSize = ivec2(320, 240);
				break;
			case FrameSize::DEPTHSD:
				mDepthSize = ivec2(480, 360);
				break;
			case FrameSize::DEPTHVGA:
				mDepthSize = ivec2(628, 468);
				break;
			}
			cStatus = mSenseMgr->EnableStream(PXCCapture::STREAM_TYPE_DEPTH, mDepthSize.x, mDepthSize.y, pFPS);
			if (cStatus >= PXC_STATUS_NO_ERROR)
			{
				mHasDepth = true;
				mShouldGetDepthAsColor = pAsColor;
				mDepthFrame = Channel16u(mDepthSize.x, mDepthSize.y);
				mDepth8uFrame = Surface8u(mDepthSize.x, mDepthSize.y, true, SurfaceChannelOrder::RGBA);
			}
		}
		return mHasDepth;
	}

	bool CinderRSSDK::start()
	{
		pxcStatus cStatus = mSenseMgr->Init();
		if (cStatus >= PXC_STATUS_NO_ERROR)
		{
			mCoordinateMapper = mSenseMgr->QueryCaptureManager()->QueryDevice()->CreateProjection();
			if (mShouldAlign)
			{
				mColorToDepthFrame = Surface8u(mRgbSize.x, mRgbSize.y, true, SurfaceChannelOrder::RGBA);
				mDepthToColorFrame = Surface8u(mRgbSize.x, mRgbSize.y, true, SurfaceChannelOrder::RGBA);
			}
			return true;
		}
		return false;
	}

	bool CinderRSSDK::update()
	{
		pxcStatus cStatus;
		if (mSenseMgr)
		{
			cStatus = mSenseMgr->AcquireFrame();
			if (cStatus < PXC_STATUS_NO_ERROR)
				return false;
			PXCCapture::Sample *mCurrentSample = mSenseMgr->QuerySample();
			if (!mCurrentSample)
				return false;
			if (mHasRgb)
			{
				if (!mCurrentSample->color)
					return false;
				PXCImage *cColorImage = mCurrentSample->color;
				PXCImage::ImageData cColorData;
				cStatus = cColorImage->AcquireAccess(PXCImage::ACCESS_READ, PXCImage::PIXEL_FORMAT_RGB32, &cColorData);
				if (cStatus < PXC_STATUS_NO_ERROR)
				{
					cColorImage->ReleaseAccess(&cColorData);
					return false;
				}
				mRgbFrame = Surface8u(reinterpret_cast<uint8_t *>(cColorData.planes[0]), mRgbSize.x, mRgbSize.y, mRgbSize.x * 4, SurfaceChannelOrder::BGRA);
				cColorImage->ReleaseAccess(&cColorData);
				if (!mHasDepth)
				{	
					mSenseMgr->ReleaseFrame();
					return true;
				}
			}
			if (mHasDepth)
			{
				if (!mCurrentSample->depth)
					return false;
				PXCImage *cDepthImage = mCurrentSample->depth;
				PXCImage::ImageData cDepthData;
				cStatus = cDepthImage->AcquireAccess(PXCImage::ACCESS_READ, PXCImage::PIXEL_FORMAT_DEPTH, &cDepthData);
				if (cStatus < PXC_STATUS_NO_ERROR)
				{
					cDepthImage->ReleaseAccess(&cDepthData);
					return false;
				}
				mDepthFrame = Channel16u(mDepthSize.x, mDepthSize.y, mDepthSize.x*sizeof(uint16_t), 1, reinterpret_cast<uint16_t *>(cDepthData.planes[0]));
				cDepthImage->ReleaseAccess(&cDepthData);

				if (mShouldGetDepthAsColor)
				{
					PXCImage::ImageData cDepth8uData;
					cStatus = cDepthImage->AcquireAccess(PXCImage::ACCESS_READ, PXCImage::PIXEL_FORMAT_RGB32, &cDepth8uData);
					if (cStatus < PXC_STATUS_NO_ERROR)
					{
						cDepthImage->ReleaseAccess(&cDepth8uData);
						return false;
					}
					mDepth8uFrame = Surface8u(reinterpret_cast<uint8_t *>(cDepth8uData.planes[0]), mDepthSize.x, mDepthSize.y, mDepthSize.x * 4, SurfaceChannelOrder::RGBA);
					cDepthImage->ReleaseAccess(&cDepth8uData);
				}

				if (!mHasRgb)
				{
					mSenseMgr->ReleaseFrame();
					return true;
				}
			}

			if (mHasDepth&&mHasRgb&&mShouldAlign&&mAlignMode==AlignMode::ALIGN_FRAME)
			{
				PXCImage *cMappedColor = mCoordinateMapper->CreateColorImageMappedToDepth(mCurrentSample->depth, mCurrentSample->color);
				PXCImage *cMappedDepth = mCoordinateMapper->CreateDepthImageMappedToColor(mCurrentSample->color, mCurrentSample->depth);

				if (!cMappedColor || !cMappedDepth)
					return false;

				PXCImage::ImageData cMappedColorData;
				if (cMappedColor->AcquireAccess(PXCImage::ACCESS_READ, PXCImage::PIXEL_FORMAT_RGB32, &cMappedColorData) >= PXC_STATUS_NO_ERROR)
				{
					mColorToDepthFrame = Surface8u(reinterpret_cast<uint8_t *>(cMappedColorData.planes[0]), mRgbSize.x, mRgbSize.y, cMappedColorData.pitches[0] * 4, SurfaceChannelOrder::RGBA);
					cMappedColor->ReleaseAccess(&cMappedColorData);
				}

				PXCImage::ImageData cMappedDepthData;
				if (cMappedDepth->AcquireAccess(PXCImage::ACCESS_READ, PXCImage::PIXEL_FORMAT_RGB32, &cMappedDepthData) >= PXC_STATUS_NO_ERROR)
				{
					mDepthToColorFrame = Surface8u(reinterpret_cast<uint8_t *>(cMappedDepthData.planes[0]), mRgbSize.x, mRgbSize.y, cMappedDepthData.pitches[0] * 4, SurfaceChannelOrder::RGBA);
					cMappedDepth->ReleaseAccess(&cMappedDepthData);
				}

				try
				{
					cMappedColor->Release();
					cMappedDepth->Release();
				}
				catch (const Exception &e)
				{
					console() << "Release check error: " << endl;
					console() << e.what() << endl;
				}
			}
			mSenseMgr->ReleaseFrame();
			return true;
		}
		return false;
	}

	bool CinderRSSDK::stop()
	{
		if (mSenseMgr)
		{
			mSenseMgr->Close();
			return true;
		}
		return false;
	}

	const Surface8u& CinderRSSDK::getRgbFrame()
	{
		return mRgbFrame;
	}

	const Channel16u& CinderRSSDK::getDepthFrame()
	{
		return mDepthFrame;
	}

	const Surface8u& CinderRSSDK::getDepth8uFrame()
	{
		return mDepth8uFrame;
	}

	const Surface8u& CinderRSSDK::getColorMappedToDepthFrame()
	{
		return mColorToDepthFrame;
	}

	const Surface8u& CinderRSSDK::getDepthMappedToColorFrame()
	{
		return mDepthToColorFrame;
	}

	//Nomenclature Notes:
	//	"Space" denotes a 3d coordinate
	//	"Image" denotes an image space point ((0, width), (0,height), (image depth))
	//	"Coords" denotes texture space (U,V) coordinates
	//  "Frame" denotes a full Surface

	//get a camera space point from a depth image point
	const vec3 CinderRSSDK::getDepthSpacePoint(float pImageX, float pImageY, float pImageZ)
	{
		if (mCoordinateMapper)
		{
			PXCPoint3DF32 cPoint;
			cPoint.x = pImageX;
			cPoint.y = pImageY;
			cPoint.z = pImageZ;

			mInPoints3D.clear();
			mInPoints3D.push_back(cPoint);
			mOutPoints3D.clear();
			mOutPoints3D.resize(2);
			mCoordinateMapper->ProjectDepthToCamera(1, &mInPoints3D[0], &mOutPoints3D[0]);
			return vec3(mOutPoints3D[0].x, mOutPoints3D[0].y, mOutPoints3D[0].z);
		}
		return vec3(0);
	}

	const vec3 CinderRSSDK::getDepthSpacePoint(int pImageX, int pImageY, uint16_t pImageZ)
	{
		return getDepthSpacePoint(static_cast<float>(pImageX), static_cast<float>(pImageY), static_cast<float>(pImageZ));
	}

	const vec3 CinderRSSDK::getDepthSpacePoint(vec3 pImageCoords)
	{
		return getDepthSpacePoint(pImageCoords.x, pImageCoords.y, pImageCoords.z);
	}

	//get a Color object from a depth image point
	const Color CinderRSSDK::getColorFromDepthImage(float pImageX, float pImageY, float pImageZ)
	{
		if (mCoordinateMapper)
		{
			PXCPoint3DF32 cPoint;
			cPoint.x = pImageX;
			cPoint.y = pImageY;
			cPoint.z = pImageZ;
			PXCPoint3DF32 *cInPoint = new PXCPoint3DF32[1];
			cInPoint[0] = cPoint;
			PXCPointF32 *cOutPoints = new PXCPointF32[1];
			mCoordinateMapper->MapDepthToColor(1, cInPoint, cOutPoints);

			float cColorX = cOutPoints[0].x;
			float cColorY = cOutPoints[0].y;

			delete cInPoint;
			delete cOutPoints;
			if (cColorX >= 0 && cColorX < mRgbSize.x&&cColorY >= 0 && cColorY < mRgbSize.y)
			{
				Color8u cColor = mRgbFrame.getPixel(ivec2(cColorX, cColorY));
				return Color(cColor);
			}
		}
		return Color::black();
	}

	const Color CinderRSSDK::getColorFromDepthImage(int pImageX, int pImageY, uint16_t pImageZ)
	{
		if (mCoordinateMapper)
			return getColorFromDepthImage(static_cast<float>(pImageX),static_cast<float>(pImageY),static_cast<float>(pImageZ));
		return Color::black();
	}

	const Color CinderRSSDK::getColorFromDepthImage(vec3 pImageCoords)
	{
		if (mCoordinateMapper)
			return getColorFromDepthImage(pImageCoords.x, pImageCoords.y, pImageCoords.z);
		return Color::black();
	}


		//get a Color object from a depth camera space point
	const Color CinderRSSDK::getColorFromDepthSpace(float pCameraX, float pCameraY, float pCameraZ)
	{
		if (mCoordinateMapper)
		{
			PXCPoint3DF32 cPoint;
			cPoint.x = pCameraX; cPoint.y = pCameraY; cPoint.z = pCameraZ;

			mInPoints3D.clear();
			mInPoints3D.push_back(cPoint);
			mOutPoints2D.clear();
			mOutPoints2D.resize(2);
			mCoordinateMapper->ProjectCameraToColor(1, &mInPoints3D[0], &mOutPoints2D[0]);

			ivec2 cColorPoint(static_cast<int>(mOutPoints2D[0].x), static_cast<int>(mOutPoints2D[0].y));
			Color8u cColor = mRgbFrame.getPixel(cColorPoint);

			return Color(cColor);
		}
		return Color::black();
	}

	const Color CinderRSSDK::getColorFromDepthSpace(vec3 pCameraPoint)
	{
		if (mCoordinateMapper)
			return getColorFromDepthSpace(pCameraPoint.x, pCameraPoint.y, pCameraPoint.z);
		return Color::black();
	}

		//get color space UVs from a depth image point
	const vec2 CinderRSSDK::getColorCoordsFromDepthImage(float pImageX, float pImageY, float pImageZ)
	{
		if (mCoordinateMapper)
		{
			PXCPoint3DF32 cPoint;
			cPoint.x = pImageX;
			cPoint.y = pImageY;
			cPoint.z = pImageZ;

			PXCPoint3DF32 *cInPoint = new PXCPoint3DF32[1];
			cInPoint[0] = cPoint;
			PXCPointF32 *cOutPoints = new PXCPointF32[1];
			mCoordinateMapper->MapDepthToColor(1, cInPoint, cOutPoints);

			float cColorX = cOutPoints[0].x;
			float cColorY = cOutPoints[0].y;

			delete cInPoint;
			delete cOutPoints;
			return vec2(cColorX / (float)mRgbSize.x, cColorY / (float)mRgbSize.y);
		}
		return vec2(0);
	}

	const vec2 CinderRSSDK::getColorCoordsFromDepthImage(int pImageX, int pImageY, uint16_t pImageZ)
	{
		return getColorCoordsFromDepthImage(static_cast<float>(pImageX), static_cast<float>(pImageY), static_cast<float>(pImageZ));
	}

	const vec2 CinderRSSDK::getColorCoordsFromDepthImage(vec3 pImageCoords)
	{
		return getColorCoordsFromDepthImage(pImageCoords.x, pImageCoords.y, pImageCoords.z);
	}

		//get color space UVs from a depth space point
	const vec2 CinderRSSDK::getColorCoordsFromDepthSpace(float pCameraX, float pCameraY, float pCameraZ)
	{
		if (mCoordinateMapper)
		{
			PXCPoint3DF32 cPoint;
			cPoint.x = pCameraX; cPoint.y = pCameraY; cPoint.z = pCameraZ;

			PXCPoint3DF32 *cInPoint = new PXCPoint3DF32[1];
			cInPoint[0] = cPoint;
			PXCPointF32 *cOutPoint = new PXCPointF32[1];
			mCoordinateMapper->ProjectCameraToColor(1, cInPoint, cOutPoint);

			vec2 cRetPt(cOutPoint[0].x / static_cast<float>(mRgbSize.x), cOutPoint[0].y / static_cast<float>(mRgbSize.y));
			delete cInPoint;
			delete cOutPoint;
			return cRetPt;
		}
		return vec2(0);
	}

	const vec2 CinderRSSDK::getColorCoordsFromDepthSpace(vec3 pCameraPoint)
	{
		return getColorCoordsFromDepthSpace(pCameraPoint.x, pCameraPoint.y, pCameraPoint.z);
	}
}