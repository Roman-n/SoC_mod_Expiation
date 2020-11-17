#ifndef	LMODEL_H
#define LMODEL_H

#include "common.h"

#ifdef USE_SJITTER

//////////////////////////////////////////////////////////////////////////////////////////
// Lighting formulas - !Infinite lights (sun). 
// Changed calulations to yield better performance and get rid of those nasty annoying bugs.
//                                         
//////////////////////////////////////////////////////////////////////////////////////////
uniform samplerCUBE         env_s3 ;
uniform samplerCUBE         env_s4 ;
float2 texBase       : TEXCOORD0 ;
float4 x : TEXCOORD0 : COLOR ;


half4 	plight_infinity	(half m, half3 point, half3 normal, half3 light_direction)       				{
  	half3 N		= normal;					// normal 
  	half3 V 	= -normalize		(point);		// vector2eye
  	half3 L 	= -light_direction;				// vector2light
	float3 Rv	= reflect  (-L,N);			
	half3 R     	= reflect         	(-V,N);
	half 	d	= saturate(dot(L,R));
		d	= saturate(dot(Rv,V));
	half    Q	= saturate(dot(-V,R));
	
		     d *= Q;		
//	half4		e0d		= texCUBE (env_s3,N);
//	half4		F4		= (Ldynamic_color.x,Ldynamic_color.y,Ldynamic_color.z,24.h);


	half4		F4		= (L_sun_color.x,L_sun_color.y,L_sun_color.z,24.h);
			
	half4		FS		= mul (m,F4);
	half4		Frs		= tex3D (s_material, FS );
			
	half4 baseColor = tex3D(s_material, L_sun_color.rgb );
    	half4 envColor  = texCUBE(env_s3, dot(L,N));
	half4 combined  = saturate(8 * envColor * baseColor);
	

	
        half fresnel = (1 - abs(dot(N,V))) * 0.6 + 0.1;  
	half glow = saturate(8 * (envColor.a * envColor.a - 0.75));

		
	float4	dl	= tex3D 			(s_material, half3(dot(L,N), d, m));	// sample material

	
	
		
		dl *= float4(lerp(L_sun_color.rgb, combined.rgb, glow), glow + fresnel) + Frs+1.7h ;
		
		dl.w     = pow(saturate(d),4);
  	return	dl;
	
}


//////////////////////////////////////////////////////////////////////////////////////////
//  !Below is model lights (fire barrels, lights, et cetera.)
//   NEW phong reflections for the spot lights and such. Used clamp instead of saturate
//   because it looks much better. No effect on performance.
//
//////////////////////////////////////////////////////////////////////////////////////////


half4 	plight_local		(half m, half3 point, half3 normal, half3 light_position, half light_range_rsq, out float rsqr)  {
  half3 N		= normal;							// normal 
  half3 L2P 	= point-light_position;                         		// light2point 
  half3 V 		= -normalize	(point);					// vector2eye
  half3 L 		= -normalize	((half3)L2P);					// vector2light
  float3	Z	= reflect	(-L,N);

  half         Zx	= saturate(dot(Z,V));


	
	half3		e0d		= texCUBE (env_s3,N);
	half4		F4		= (e0d.x,e0d.y,e0d.z,1.h);
			
	half4		FS		= mul (m,F4);
	half4		Frs		= tex3D (s_material, FS );
			
	half4 baseColor = tex3D(s_material, Ldynamic_color );
 	half4 envColor  = texCUBE(env_s3, dot(L,N));
	half4 combined  = saturate(4 * envColor * baseColor);
	

	
        float fresnel = (1 - abs(dot(N,V))) * 0.6 + 0.1;  
	float glow = saturate(4 * (envColor.a * envColor.a - 0.8));
	half4  FX2  = (lerp( Ldynamic_color.rgb,combined.rgb, glow),glow + fresnel + Frs);


		rsqr	= dot		(L2P,L2P);					// distance 2 light (squared)
  half  att 	= saturate	(1 - rsqr*light_range_rsq);			// q-linear attenuate
  half4 light	= tex3D		(s_material, half3(dot(L,N), Zx, m)); 	              // sample material
 

	light.w = pow(saturate(Zx),4);

  	 return (att)*(light*FX2);



}

 half4	blendp	(half4	value, float4 	tcp)    		{
	

	#ifndef FP16_BLEND  
		value 	+= (half4)tex2Dproj 	(s_accumulator, tcp); 	// emulate blend
	#endif
	return 	value	;
 }
 half4	blend	(half4	value, float2 	tc)			{
	#ifndef FP16_BLEND  
		value 	+= (half4)tex2D 	(s_accumulator, tc); 	// emulate blend
	#endif
	return 	value	;
 }










#else


//////////////////////////////////////////////////////////////////////////////////////////
// Lighting formulas			// 
half4 	plight_infinity		(half m, half3 point, half3 normal, half3 light_direction)       				{
  half3 N		= normal;							// normal 
  half3 V 		= -normalize	(point);					// vector2eye
  half3 L 		= -light_direction;						// vector2light
  half3 H		= normalize	(L+V);						// half-angle-vector 
  return tex3D 		(s_material,	half3( dot(L,N), dot(H,N), m ) );		// sample material
}
half4 	plight_infinity2	(half m, half3 point, half3 normal, half3 light_direction)       				{
  	half3 N		= normal;							// normal 
  	half3 V 	= -normalize		(point);		// vector2eye
  	half3 L 	= -light_direction;					// vector2light
 	half3 H		= normalize			(L+V);			// half-angle-vector 
	half3 R     = reflect         	(-V,N);
	half 	s	= saturate(dot(L,R));
			s	= saturate(dot(H,N));
	half 	f 	= saturate(dot(-V,R));
			s  *= f;
	half4	r	= tex3D 			(s_material,	half3( dot(L,N), s, m ) );	// sample material
			r.w	= pow(saturate(s),4);
  	return	r	;
}
half4 	plight_local		(half m, half3 point, half3 normal, half3 light_position, half light_range_rsq, out float rsqr)  {
  half3 N		= normal;							// normal 
  half3 L2P 	= point-light_position;                         		// light2point 
  half3 V 		= -normalize	(point);					// vector2eye
  half3 L 		= -normalize	((half3)L2P);					// vector2light
  half3 H		= normalize	(L+V);						// half-angle-vector
		rsqr	= dot		(L2P,L2P);					// distance 2 light (squared)
  half  att 	= saturate	(1 - rsqr*light_range_rsq);			// q-linear attenuate
  half4 light	= tex3D		(s_material, half3( dot(L,N), dot(H,N), m ) ); 	// sample material
  return att*light;
}

half4	blendp	(half4	value, float4 	tcp)    		{
	#ifndef FP16_BLEND  
		value 	+= (half4)tex2Dproj 	(s_accumulator, tcp); 	// emulate blend
	#endif
	return 	value	;
}
half4	blend	(half4	value, float2 	tc)			{
	#ifndef FP16_BLEND  
		value 	+= (half4)tex2D 	(s_accumulator, tc); 	// emulate blend
	#endif
	return 	value	;
}


#endif

#endif
