#version 330 core
out vec4 oColor;

in vec3 fColor;
in vec3 fNormal;

void main()
{
	vec3 normal = normalize( -fNormal );
	float diffuse = max( dot( normal, vec3( 0, 0, -1 ) ), 0 );
	oColor = vec4(vec3(fColor) * diffuse, 1.0);
}
