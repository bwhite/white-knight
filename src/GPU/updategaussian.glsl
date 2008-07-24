uniform sampler2DRect one_sampler;
uniform sampler2DRect two_sampler;

uniform float alpha;

void main()
{
	vec4 one = texture2DRect(one_sampler, gl_FragCoord.xy);
	vec4 two = texture2DRect(two_sampler, gl_FragCoord.xy);
	
	vec3 mean = one.rgb;
	vec3 curr = two.rgb;
	float var = one.a;
	
	float sum_square_diff = dot(mean - curr, mean - curr); 
	
	float nu = exp(-0.5 * sum_square_diff / var) / pow(2.0*3.14159264*var, 1.5);
	float rho = nu * alpha;
	
	gl_FragData[0].rgb = mix(mean, curr, rho);
	gl_FragData[0].a = mix(var, sum_square_diff, rho);
}
