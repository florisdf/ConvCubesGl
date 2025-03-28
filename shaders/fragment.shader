#version 330 core
out vec4 oColor;

in vec4 fColor;
in vec3 fNormal;

void main()
{
    vec3 normal = normalize( -fNormal );
    float diffuse = max( dot( normal, vec3( 0, 0, -1 ) ), 0 );
    float w_diff = 0.9;
    float w_amb = 0.1;
    vec3 rgbColor = vec3(fColor.x, fColor.y, fColor.z);
    oColor = vec4(w_amb * rgbColor + w_diff * rgbColor * diffuse, fColor.w);
}
