#version 150

uniform sampler2D mTexRgb;
in vec2 UV;
out vec4 oColor;

void main()
{
	oColor = texture2D(mTexRgb, vec2(UV.x,1.0-UV.y));
}