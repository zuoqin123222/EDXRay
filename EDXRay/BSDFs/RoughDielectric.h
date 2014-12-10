#pragma once

#include "../Core/BSDF.h"

namespace EDX
{
	namespace RayTracer
	{
		class RoughDielectric : public BSDF
		{
		private:
			float mRoughness;
			float mEtai, mEtat;

			static const ScatterType ReflectScatter = ScatterType(BSDF_REFLECTION | BSDF_GLOSSY);
			static const ScatterType RefractScatter = ScatterType(BSDF_TRANSMISSION | BSDF_GLOSSY);

		public:
			RoughDielectric(const Color reflectance = Color::WHITE, float roughness = 0.06f, float etai = 1.0f, float etat = 1.5f)
				: BSDF(ScatterType(BSDF_REFLECTION | BSDF_TRANSMISSION | BSDF_GLOSSY), BSDFType::RoughDielectric, reflectance)
				, mEtai(etai)
				, mEtat(etat)
				, mRoughness(roughness)
			{
			}
			RoughDielectric(const char* pFile, float roughness = 0.05f, float etai = 1.0f, float etat = 1.5f)
				: BSDF(ScatterType(BSDF_REFLECTION | BSDF_TRANSMISSION | BSDF_GLOSSY), BSDFType::RoughDielectric, pFile)
				, mEtai(etai)
				, mEtat(etat)
				, mRoughness(roughness)
			{
			}

			Color SampleScattered(const Vector3& _wo,
				const Sample& sample,
				const DifferentialGeom& diffGeom,
				Vector3* pvIn, float* pPdf,
				ScatterType types = BSDF_ALL,
				ScatterType* pSampledTypes = NULL) const;

		private:
			float Pdf(const Vector3& wo, const Vector3& wi, ScatterType types = BSDF_ALL) const
			{
				bool sampleReflect = (types & ReflectScatter) == ReflectScatter;
				bool sampleRefract = (types & RefractScatter) == RefractScatter;
				bool reflect = BSDFCoordinate::CosTheta(wi) * BSDFCoordinate::CosTheta(wo) > 0.0f;

				Vector3 wh;
				float dwh_dwi;
				if (reflect)
				{
					if ((ReflectScatter & types) != ReflectScatter)
						return 0.0f;

					wh = Math::Normalize(wo + wi);
					dwh_dwi = 1.0f / (4.0f * Math::Dot(wi, wh));

				}
				else
				{
					if ((RefractScatter & types) != RefractScatter)
						return 0.0f;

					float eta = BSDFCoordinate::CosTheta(wo) > 0.0f ? mEtat / mEtai : mEtat / mEtai;
					wh = Math::Normalize(wo + wi * eta);

					const float ODotH = Math::Dot(wo, wh), IDotH = Math::Dot(wi, wh);
					float sqrtDenom = ODotH + eta * IDotH;
					dwh_dwi = (eta * eta * IDotH) / (sqrtDenom * sqrtDenom);
				}
				if (BSDFCoordinate::CosTheta(wh) < 0.0f)
					wh *= -1.0f;

				float whProb = GGX_Pdf(wh, mRoughness);
				if (sampleReflect && sampleRefract)
				{
					float F = BSDF::FresnelDielectric(Math::Dot(wo, wh), mEtai, mEtat);
					whProb *= reflect ? F : 1.0f - F;
				}

				return Math::Abs(whProb * dwh_dwi);
			}

			float Eval(const Vector3& wo, const Vector3& wi, ScatterType types = BSDF_ALL) const
			{
				bool reflect = BSDFCoordinate::CosTheta(wi) * BSDFCoordinate::CosTheta(wo) > 0.0f;
				float eta = BSDFCoordinate::CosTheta(wo) > 0.0f ? mEtat / mEtai : mEtat / mEtai;

				Vector3 wh;
				if (reflect)
				{
					if ((ReflectScatter & types) != ReflectScatter)
						return 0.0f;

					wh = Math::Normalize(wo + wi);
				}
				else
				{
					if ((RefractScatter & types) != RefractScatter)
						return 0.0f;

					wh = Math::Normalize(wo + wi * eta);
				}
				if (BSDFCoordinate::CosTheta(wh) < 0.0f)
					wh *= -1.0f;

				float D = GGX_D(wh, mRoughness);
				if (D == 0.0f)
					return 0.0f;

				float F = BSDF::FresnelDielectric(Math::Dot(wo, wh), mEtai, mEtat);
				float G = GGX_G(wo, wi, wh, mRoughness);

				if (reflect)
				{
					return F * D * G / (4.0f * BSDFCoordinate::AbsCosTheta(wi) * BSDFCoordinate::AbsCosTheta(wo));
				}
				else
				{
					const float ODotH = Math::Dot(wo, wh), IDotH = Math::Dot(wi, wh);

					float sqrtDenom = ODotH + eta * IDotH;
					float value = ((1 - F) * D * G * eta * eta * ODotH * IDotH) /
						(BSDFCoordinate::CosTheta(wo) * sqrtDenom * sqrtDenom);

					// TODO: Fix solid angle compression when tracing radiance
					float factor = 1.0f;

					return Math::Abs(value * factor * factor);
				}
			}
		};
	}
}