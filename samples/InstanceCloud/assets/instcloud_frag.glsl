#version 150

uniform vec3 ViewDirection;

in vec4 Color;
in vec3 Normal;

out vec4 oColor;
void main()
{
	vec3 nView = normalize(ViewDirection);
	vec3 nNorm = normalize(Normal);
	float nContrib = dot(Normal, ViewDirection);


	oColor=Color*nContrib+(Color*0.5);
}