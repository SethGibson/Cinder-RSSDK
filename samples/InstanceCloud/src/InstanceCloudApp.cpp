#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Batch.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/Camera.h"
#include "cinder/MayaCamUI.h"
#include "CiRSSDK.h"

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace CiRSSDK;

static ivec2 S_DIMS(320, 240);
static int S_STEP = 1;

class InstanceCloudApp : public App
{
public:
	void setup() override;
	void mouseDown(MouseEvent event) override;
	void mouseDrag(MouseEvent event) override;
	void update() override;
	void draw() override;

	void exit();
	struct InstData
	{
		vec3 IPosition;
		vec4 IColor;
		float ISize;
		InstData(vec3 pPos, vec4 pCol, float pSize) :IPosition(pPos), IColor(pCol), ISize(pSize){}
	};

private:
	void setupRSSDK();
	void setupMesh();

	size_t	mBufferSize;
	gl::VboRef mInstanceData;
	gl::VboMeshRef mVboMesh;
	geom::BufferLayout mInstanceAttribs;
	geom::Sphere mSphere;
	gl::GlslProgRef mShader;
	gl::BatchRef mDrawObj;

	vector<InstData> mInst;
	CinderRSRef mCinderRS;

	CameraPersp mCamera;
	MayaCamUI mMaya;
};

void InstanceCloudApp::setup()
{
	getWindow()->setSize(1280, 720);
	setFrameRate(60);
	setupRSSDK();
	setupMesh();
}

void InstanceCloudApp::setupRSSDK()
{
	mCinderRS = CinderRSSDK::create();

	mCinderRS->init();
	mCinderRS->initRgb(FrameSize::RGBVGA, 60);
	mCinderRS->initDepth(FrameSize::DEPTHQVGA, 60, false);
	mCinderRS->start();
	getSignalCleanup().connect(std::bind(&InstanceCloudApp::exit, this));
}

void InstanceCloudApp::setupMesh()
{
	mBufferSize = mCinderRS->getDepthWidth()*mCinderRS->getDepthHeight()*sizeof(InstData);
	try
	{
		mShader = gl::GlslProg::create(loadAsset("instcloud_vert.glsl"), loadAsset("instcloud_frag.glsl"));
	}
	catch (const gl::GlslProgExc &e)
	{
		console() << "Error loading shaders: " << endl;
		console() << e.what() << endl;
	}

	mSphere = geom::Sphere().radius(0.25);
	mVboMesh = gl::VboMesh::create(mSphere);

	for (int vy = 0; vy < S_DIMS.y; vy += S_STEP)
	{
		for (int vx = 0; vx < S_DIMS.x; vx += S_STEP)
		{
			mInst.push_back(InstData(vec3(0), vec4(0), 1.0f));
		}
	}
	mInstanceData = gl::Vbo::create(GL_ARRAY_BUFFER, mInst, GL_DYNAMIC_DRAW);
	mInstanceAttribs.append(geom::CUSTOM_0, 3, sizeof(InstData), offsetof(InstData, IPosition), 1);
	mInstanceAttribs.append(geom::CUSTOM_1, 4, sizeof(InstData), offsetof(InstData, IColor), 1);
	mInstanceAttribs.append(geom::CUSTOM_2, 1, sizeof(InstData), offsetof(InstData, ISize), 1);

	mVboMesh->appendVbo(mInstanceAttribs, mInstanceData);
	mDrawObj = gl::Batch::create(mVboMesh, mShader, { { geom::CUSTOM_0, "iPosition" }, { geom::CUSTOM_1, "iColor" }, { geom::CUSTOM_2, "iSize" } });

	mCamera.setPerspective(45.0f, getWindowAspectRatio(), 10.0f, 4000.0f);
	mCamera.lookAt(vec3(0), vec3(0, 0, 1), vec3(0, 1, 0));
	mCamera.setCenterOfInterestPoint(vec3(0, 0, 250));

	mMaya.setCurrentCam(mCamera);
}

void InstanceCloudApp::mouseDown(MouseEvent event)
{
	mMaya.mouseDown(event.getPos());
}

void InstanceCloudApp::mouseDrag(MouseEvent event)
{
	mMaya.mouseDrag(event.getPos(), event.isLeftDown(), false, event.isRightDown());
}

void InstanceCloudApp::update()
{
	mCinderRS->update();
	mInst.clear();
	Channel16u cDepth = mCinderRS->getDepthFrame();
	Channel16u::Iter cDepthIter = cDepth.getIter();

	while (cDepthIter.line())
	{
		while (cDepthIter.pixel())
		{
			float cVal = (float)cDepthIter.v();
			if (cVal>100 && cVal < 1000)
			{
				vec3 cWorld = mCinderRS->getDepthSpacePoint(vec3(cDepthIter.x(), cDepthIter.y(), cVal));
				Color cColor = mCinderRS->getColorFromDepthSpace(cWorld);
				mInst.push_back(InstData(cWorld, vec4(cColor.r, cColor.g, cColor.b, 1), 1.0f));
			}
		}
	}

	mInstanceData->bufferData(mInst.size()*sizeof(InstData), mInst.data(), GL_DYNAMIC_DRAW);
	mVboMesh->gl::VboMesh::create(mSphere);
	mVboMesh->appendVbo(mInstanceAttribs, mInstanceData);
	mDrawObj->replaceVboMesh(mVboMesh);
}

void InstanceCloudApp::draw()
{
	gl::clear(Color(0.1f, 0.25f, 0));
	gl::enableDepthRead();
	gl::enableDepthWrite();
	gl::setMatrices(mMaya.getCamera());
	mDrawObj->getGlslProg()->uniform("ViewDirection", mMaya.getCamera().getViewDirection());
	mDrawObj->drawInstanced(mInst.size());
}

void InstanceCloudApp::exit()
{
	mCinderRS->stop();
}

CINDER_APP(InstanceCloudApp, RendererGl)
