

static const float PI = 3.1415926535897932384626433832795f;
// preserving the energy in hemisphere
static const float sgMinLambda = 4.60517f;
static const int MAX_REFLECTOR_NUM = 3;

// Spherical Gaussian
struct SG
{
	// lobe axis
	float3 axis;
	// bandwidth
	float lambda;
	// amplitude
	float mu;
};

struct SGReflectors
{
	// reflector indices
	int indices[MAX_REFLECTOR_NUM];
};

// product integral of 2 SGs
float sgProductIntegral(SG sg1, SG sg2)
{
	float3 p1 = sg1.axis;
	float3 p2 = sg2.axis;

	float l1 = sg1.lambda;
	float l2 = sg2.lambda;

	float c1 = (l1 * l2) / (l1 + l2);
	float c2 = dot(p1, p2);
	float l3 = l1 + l2 - c1 * (1 - c2);

	float factor = c1 * (c2 - 1);
	float ret;
	if (factor < -10)
	{
		ret = 0;
	}
	else
	{
		ret = exp(factor) * (sg1.mu * sg2.mu * 2 * PI) / l3;
	}

	return ret;
}

// dot product of 2 SGs
float sgDot(SG sg1, SG sg2)
{
	return sgProductIntegral(sg1, sg2);
}

// integral of SG
float sgIntegral(float lambda, float mu)
{
	float ret;
	if (lambda >= sgMinLambda)
	{
		ret = mu * 2 * PI / lambda;
	}
	else
	{
		ret = mu * 2 * PI * (1 - exp(-2 * lambda)) / lambda;
	}

	return ret;
}

// calculate bandwidth for ASG with given extent
float asgCalcBandwidth(float dr)
{
	float dr2 = dr * dr;
	float bw = max(sgMinLambda, -(((1 + dr2) * (-5.991f + log(1 + dr2))) / (2 * dr2)));
	return bw;
}

// calculate bandwidth for SG with given extent
float sgCalcBandwidth(float dr, float nol)
{
	float logEps = -4.60517f;
	float drNol = dr * nol;
	float drNol2 = drNol * drNol;
	float bw = logEps / (1 - sqrt(1 + drNol2)) - logEps;
	return bw;
}

// calculate amplitude for SG with given extent
float sgCalcAmplitude(float lambda, float totalEnergy)
{
	float sgEnergy = sgIntegral(lambda, 1);
	float mu = totalEnergy / sgEnergy;
	return mu;
}

// reflect vector 'viewOrLight' by vector 'halfDir'
float3 gReflectVector(float3 viewOrLight, float3 halfDir)
{
	float3 o = normalize(viewOrLight);
	float3 h = normalize(halfDir);
	float3 i = normalize(2 * dot(o, h) * h - o);

	return i;
}

// approximating NDF as SG
SG sgNDF(float roughness, float3 lightDir, float3 viewDir, float3 normalDir)
{
	float3 halfDir = normalize((lightDir + viewDir) * .5f);
	float3 reflectDir = gReflectVector(viewDir, normalDir);
	
	// Jacobian determinant for differential area
	float jacobian = max(4*dot(halfDir, viewDir), 0.001f);

	// contruct warpped SG
	float m2 = roughness * roughness;
	SG sg;
	sg.axis = reflectDir;
	sg.lambda = (2 / (m2* jacobian));
	sg.mu = 1 / (PI * m2);

	return sg;
}

// integrating reflected radiance(from a disk to a sphere)
float gIntegrateDiskLighting(float3 shadingPt, float3 diskPt, float3 diskNormal, float diskRadius,
	float3 lightDir, float lightIntensity, float roughness)
{
	float m = roughness;
	float m2 = m * m;
	float3 n = diskNormal;
	float3 l = lightDir;
	float nol = max(dot(n, l), 0);
	float dr = diskRadius / length(diskPt - shadingPt);

	float k = (0.288f * nol) / m2 - 0.673f;
	float lighting = 0;
	if (k != 0)
	{
		float drCos = dr * nol;
		float drCos2 = drCos * drCos;
		float approxSinThetaA2 = drCos2 / (1 + drCos2);
		float midTerm = (1 - exp(-k * approxSinThetaA2)) / (2 * k);

		float gDGGX0 = 1 / (m2 * PI);
		float lyr = PI * m2 * gDGGX0 * nol;
		lighting = lightIntensity * midTerm * lyr * PI / k;
	}

	return lighting;
}

// approximating SG light
SG sgReflectLight(float3 shadingPt, float3 lightDir, float lightIntensity, float roughness,
	float3 diskCenter, float3 diskNormal, float diskRadius)
{
	float nol = dot(diskNormal, lightDir);
	float shadingDist = length(shadingPt - diskCenter);
	float dr = diskRadius / shadingDist;
	float3 refViewDir = normalize(shadingPt - diskCenter);
	float minorSize = dot(diskNormal, refViewDir)* diskRadius / shadingDist;

	SG sgLight;
	sgLight.axis = -refViewDir;
	sgLight.lambda = 2 * asgCalcBandwidth(minorSize);
	//sgLight.lambda = sgCalcBandwidth(diskRadius / shadingDist, nol);
	float sgEnergy = gIntegrateDiskLighting(shadingPt, diskCenter, diskNormal, diskRadius,
		lightDir, lightIntensity, roughness);
	sgLight.mu = sgCalcAmplitude(sgLight.lambda, sgEnergy);

	return sgLight;
}

// find specular peak
float3 gCalcPeakPoint(float3 normalDir, float3 planePt, float3 lightDir, float3 viewPt)
{
	float3 halfDir = normalDir;
	float3 viewDir = gReflectVector(lightDir, halfDir);
	float viewProjDist = dot(normalize(viewPt - planePt), normalDir) * length(viewPt - planePt);
	float viewPeakDist = viewProjDist / dot(lightDir, halfDir);
	float3 peakPt = viewPt - viewDir * viewPeakDist;

	return peakPt;
}

// instersection area of a disk and a rectangle
float gCircIntsRectArea(float3 circCenter, float circRadius,
	float3 rectCenter, float3 rMajorAxis, float3 rMinorAxis, float rectMajorRadius, float rectMinorRadius)
{
	float2 rCenter = float2(0, 0);
	float rLeft = -rectMajorRadius;
	float rRight = rectMajorRadius;
	float rTop = rectMinorRadius;
	float rBottom = -rectMinorRadius;

	float cDistX = dot(circCenter - rectCenter, rMajorAxis);
	float cDistY = dot(circCenter - rectCenter, rMinorAxis);
	float cLeft = cDistX - circRadius;
	float cRight = cDistX + circRadius;
	float cTop = cDistY + circRadius;
	float cBottom = cDistY - circRadius;

	float xOverlap = max(0, min(rRight, cRight) - max(rLeft, cLeft));
	float yOverlap = max(0, min(rTop, cTop) - max(rBottom, cBottom));

	float area = xOverlap * yOverlap;
	return area;
}

// calculate incident illumation from a specular reflection
float3 sgReflectShading(
	// shading point
	float3 shadingPt, float3 inShadingNormal, float shadingRoughness, float samplingRadius,
	// reflection plane
	float3 planeCenter, float3 inPlaneNormal, float3 inPlaneMajorAxis, float3 planeSpecularColor,
	float planeMajorRadius, float planeMinorRadius, float planeRoughness,
	// light
	float3 inLightDir, float lightIntensity, float3 viewPoint)
{
	// avoid warning X4000
	bool bIsBlack = false;

	if (planeMajorRadius < 0.1f || planeMinorRadius < 0.1f)
	{
		bIsBlack = true;
	}

	float3 shadingNormal = normalize(inShadingNormal);
	float3 planeNormal = normalize(inPlaneNormal);
	// need validating axes in cpu
	float3 planeMajorAxis = normalize(inPlaneMajorAxis);
	float3 planeMinorAxis = normalize(cross(planeMajorAxis, planeNormal));
	float3 lightDir = normalize(inLightDir);
	float3 viewDir = normalize(viewPoint - shadingPt);

	// condition check in gCalcPeakPoint
	float3 reflHalfDir = planeNormal;
	if (dot(lightDir, reflHalfDir) < 0.001f)
	{
		bIsBlack = true;
	}

	// find specular peak
	float3 reflPeakPt = gCalcPeakPoint(planeNormal, planeCenter, lightDir, shadingPt);

	float reflDist = length(shadingPt - reflPeakPt);
	if (reflDist < 0.001f)
	{
		bIsBlack = true;
	}

	// approximating reflection light
	SG reflSgLight = sgReflectLight(shadingPt, lightDir, lightIntensity,
		planeRoughness, reflPeakPt, planeNormal, samplingRadius);

	SG shadingNDF = sgNDF(shadingRoughness, reflSgLight.axis, viewDir, shadingNormal);
	float reflShading = sgDot(reflSgLight, shadingNDF);

	// intersection area
	float intsArea = gCircIntsRectArea(reflPeakPt, samplingRadius, planeCenter,
		planeMajorAxis, planeMinorAxis, planeMajorRadius, planeMinorRadius);
	float intsPercent = min(1, intsArea/ (4 * samplingRadius * samplingRadius));

	float reflPower = reflShading * intsPercent;
	//float reflPower = reflShading;
	float3 reflColor = planeSpecularColor * reflPower;
	if (bIsBlack)
	{
		reflColor = float3(0, 0, 0);
	}

	return reflColor;
}

// calculate glossy reflections
float3 sgGlossyReflection(float3 shadingPt, float3 inShadingNormal, float shadingRoughness,
	float3 inLightDir, float lightIntensity, float3 viewPoint, SGReflectors reflectors)
{
	float samplingRadius = specularSamplingRadius;
	float3 defaultSpecularColor = float3(1.f, 1.f, 1.f);

	float3 reflColor = 0;
	for (int i = 0; i < MAX_REFLECTOR_NUM; i++)
	{
		int plId = reflectors.indices[i];
		if (plId >= 0)
		{
			reflColor += sgReflectShading(
				// shading point
				shadingPt, inShadingNormal, shadingRoughness, samplingRadius,
				// reflection plane
				plCenter_DifX[plId].xyz, plNorm_DifY[plId].xyz, plAxis_DifZ[plId].xyz, defaultSpecularColor,
				plElement0[plId].z, plElement0[plId].w, plElement0[plId].y,
				// light
				inLightDir, lightIntensity, viewPoint
			);
		}

	}

	return reflColor;
}

// calculate projection point from a shading point to a rectangle
float3 gCalcProjPoint(float3 planeNormal, float3 planePt, float3 shadingPt)
{
	float3 dir0 = planePt - shadingPt;
	float3 projDir = dot(dir0, -planeNormal);
	float projDist = length(projDir);
	float3 projPt = shadingPt - planeNormal * projDist;

	return projPt;
}

// integrating total reflected diffuse from a disk
float gIntegrateDiskDiffuse(float dr)
{
	float dr2 = dr * dr;
	float approxD = 0.666667f * PI * dr2 / (1 + dr2);
	return approxD;
}

// calculate diffuse reflection
float3 gReflectDiffuse(
	// shading point
	float3 shadingPt, float samplingRadius,
	// reflection plane
	float3 planeCenter, float3 inPlaneNormal, float3 inPlaneMajorAxis, 
	float planeMajorRadius, float planeMinorRadius, float3 planeDiffuseColor,
	// light
	float3 inLightDir, float lightIntensity)
{
	float3 planeNormal = normalize(inPlaneNormal);
	// need validating axes in cpu
	float3 planeMajorAxis = normalize(inPlaneMajorAxis);
	float3 planeMinorAxis = normalize(cross(planeMajorAxis, planeNormal));
	float3 lightDir = normalize(inLightDir);

	float3 projPt = gCalcProjPoint(planeNormal, planeCenter, shadingPt);

	// integrating area
	float shadingDist = length(projPt - shadingPt);
	float dr = samplingRadius / shadingDist;
	float integratedDiffuse = gIntegrateDiskDiffuse(dr);

	float planeNoL = max(dot(planeNormal, lightDir), 0);

	// intersection area
	float intsArea = gCircIntsRectArea(projPt, samplingRadius, planeCenter,
		planeMajorAxis, planeMinorAxis, planeMajorRadius, planeMinorRadius);
	float intsPercent = min(1, intsArea / (4 * samplingRadius * samplingRadius));

	float lighting = integratedDiffuse * planeNoL * lightIntensity;

	return lighting * planeDiffuseColor;
}

// calculate diffuse reflection
float3 gDiffuseReflection(float3 shadingPt, float3 inLightDir, float lightIntensity, SGReflectors reflectors)
{
	float samplingRadius = diffuseSamplingRadius;

	float3 reflColor = 0;
	for (int i = 0; i < MAX_REFLECTOR_NUM; i++)
	{
		int pid = reflectors.indices[i];
		if (pid >= 0)
		{
			reflColor += gReflectDiffuse(
				// shading point
				shadingPt, samplingRadius,
				// reflection plane
				plCenter_DifX[pid].xyz, plNorm_DifY[pid].xyz, plAxis_DifZ[pid].xyz,
				plElement0[pid].z, plElement0[pid].w, float3(plCenter_DifX[pid].w, plNorm_DifY[pid].w, plAxis_DifZ[pid].w),
				// light
				inLightDir, lightIntensity
			);
		}
	}

	return reflColor;
}

// TODO: reflector indices can be cached in a texture
SGReflectors findDiffuseRelatedPlanes(float3 shadingPt, float3 inShadingNormal)
{
	SGReflectors reflectors;
	int tempArray[MAX_REFLECTOR_NUM] = { -1, -1, -1 };

	int reflectorNum = 0;
	[unroll]
	for (int i = 0; i < MAX_RELATED_PLANE_NUM; i++)
	{
		int plId = int(mRelatedPlanes[i].x + .1f);
		if (plId < 0)
			break;

		float3 plCenter = plCenter_DifX[plId].xyz;
		float3 plNormal = plNorm_DifY[plId].xyz;

		float3 projPt = gCalcProjPoint(plNormal, plCenter, shadingPt);
		float3 viewDir = shadingPt - projPt;
		float3 viewDirNm = normalize(viewDir);
		float viewDist = length(viewDir);

		if (dot(viewDir, plNormal) < 0)
			continue;

		if (reflectorNum == MAX_REFLECTOR_NUM)
			break;

		tempArray[reflectorNum] = plId;
		reflectorNum++;
	}

	reflectors.indices = tempArray;
	return reflectors;
}

// TODO: reflector indices can be cached in a texture
SGReflectors findSpecularRelatedPlanes(float3 shadingPt, float3 inShadingNormal, float3 lightDir)
{
	SGReflectors reflectors;
	int tempArray[MAX_REFLECTOR_NUM] = { -1, -1, -1 };

	int reflectorNum = 0;
	[unroll]
	for (int i = 0; i < MAX_RELATED_PLANE_NUM; i++)
	{
		int plId = int(mRelatedPlanes[i].x + .1f);
		if (plId < 0)
			break;

		float roughness = plElement0[plId].y;
		if (roughness > .3f)
			continue;

		float3 plCenter = plCenter_DifX[plId].xyz;
		float3 plNormal = plNorm_DifY[plId].xyz;

		float3 projPt = gCalcProjPoint(plNormal, plCenter, shadingPt);
		float3 viewDir = shadingPt - projPt;
		float3 viewDirNm = normalize(viewDir);
		float viewDist = length(viewDir);

		if (dot(viewDir, plNormal) < 0 || dot(lightDir, plNormal) < 0)
			continue;

		if (reflectorNum == MAX_REFLECTOR_NUM)
			break;

		tempArray[reflectorNum] = plId;
		reflectorNum++;
	}

	reflectors.indices = tempArray;
	return reflectors;
}

// calculate total indirect lighting
float3 GILighting(float3 shadingPt, float3 inShadingNormal, float shadingRoughness,
	float3 inLightDir, float3 viewPoint)
{
	bool bShowSpecularGI = mToggleOptionsA[0];
	bool bShowDiffuseGI = mToggleOptionsA[3];
	float3 shadingNormal = normalize(inShadingNormal);
	float3 lightDir = normalize(inLightDir);

	float3 OutColor = 0;
	if (bShowSpecularGI)
	{
		float GIIntensity = LightIntensity;
		SGReflectors reflectors = findSpecularRelatedPlanes(shadingPt, shadingNormal, lightDir);
		float3 reflColor = sgGlossyReflection(shadingPt, shadingNormal, shadingRoughness,
			inLightDir, GIIntensity, viewPoint, reflectors);
		OutColor += reflColor;
	}

	if (bShowDiffuseGI)
	{
		float GIIntensity = LightIntensity * DiffuseReflIntensity;
		SGReflectors reflectors = findDiffuseRelatedPlanes(shadingPt, shadingNormal);
		float3 reflColor = gDiffuseReflection(shadingPt, inLightDir, GIIntensity, reflectors);
		OutColor += reflColor;
	}

	return OutColor;
}
