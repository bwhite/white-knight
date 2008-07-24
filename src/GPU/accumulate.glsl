uniform sampler2DRect accum_sampler;
uniform sampler2DRect point_sampler;
uniform sampler2DRect pixel_sampler;

uniform float window;
uniform vec3 var;

#define PI (3.141592654)

void main()
{
	float accum = texture2DRect(accum_sampler, gl_FragCoord.xy).r;
	vec3 point = texture2DRect(point_sampler, gl_FragCoord.xy).rgb;
	vec3 pixel = texture2DRect(pixel_sampler, gl_FragCoord.xy).rgb;
	
	float coeff = 1.0 / pow(2.0*PI*(var.x * var.y * var.z), 3.0/2.0);
	vec3 exponent = -(point - pixel) * (point - pixel) / (2.0 * var);
	
	float prob = coeff * exp(exponent.x + exponent.y + exponent.z);
				
	gl_FragData[0].r = accum + prob / window;
}