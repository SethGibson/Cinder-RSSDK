#include <memory>
#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "CiRSSDK.h"

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace CiRSSDK;

class BasicApp : public App
{
public:
	void setup() override;
	void mouseDown( MouseEvent event ) override;
	void update() override;
	void draw() override;
	void cleanup() override;

private:
	CinderRSRef	mCinderRS;
	gl::Texture2dRef	mTexRgb;
	gl::Texture2dRef	mTexDepth;
};

void BasicApp::setup()
{
	getWindow()->setSize(1280, 480);
	setFrameRate(30);

	mCinderRS = CinderRSSDK::create();
	mCinderRS->init();
	mCinderRS->initDepth(FrameSize::DEPTHQVGA, 30, true);
	mCinderRS->initRgb(FrameSize::RGBVGA, 30);

	mCinderRS->start();

	mTexRgb = gl::Texture2d::create(mCinderRS->getRgbWidth(), mCinderRS->getRgbHeight());
	mTexDepth = gl::Texture2d::create(mCinderRS->getDepthWidth(), mCinderRS->getDepthHeight());
}

void BasicApp::mouseDown( MouseEvent event )
{
}

void BasicApp::update()
{
	mCinderRS->update();
	mTexRgb->update(mCinderRS->getRgbFrame());
	mTexDepth->update(mCinderRS->getDepth8uFrame());
}

void BasicApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) ); 
	if (mTexRgb)
		gl::draw(mTexRgb, vec2(0));
	if (mTexDepth)
		gl::draw(mTexDepth, Rectf({ vec2(640, 0), vec2(1280, 480) }));
}

void BasicApp::cleanup()
{
	mCinderRS->stop();
}

CINDER_APP( BasicApp, RendererGl )
