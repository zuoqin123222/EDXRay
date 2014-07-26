#include "Camera.h"
#include "Sampler.h"
#include "Sampling.h"


namespace EDX
{
	namespace RayTracer
	{
		void Camera::Init(const Vector3& pos,
			const Vector3& tar,
			const Vector3& up,
			const int resX,
			const int resY,
			const float FOV,
			const float nearClip,
			const float farClip,
			const float lensR,
			const float focalDist)
		{
			EDX::Camera::Init(pos, tar, up, resX, resY, FOV, nearClip, farClip);

			mLensRadius = lensR;
			mFocalPlaneDist = focalDist;
		}

		void Camera::GenerateRay(const CameraSample& sample, Ray* pRay) const
		{
			Vector3 camCoord = Matrix::TransformPoint(Vector3(sample.imageX, sample.imageY, 0.0f), mRasterToCamera);

			pRay->mOrg = Vector3::ZERO;
			pRay->mDir = Math::Normalize(camCoord);

			if (mLensRadius > 0.0f)
			{
				float fFocalHit = mFocalPlaneDist / -pRay->mDir.z;
				Vector3 ptFocal = pRay->CalcPoint(fFocalHit);

				float fU, fV;
				Sampling::ConcentricSampleDisk(sample.lensU, sample.lensV, &fU, &fV);
				fU *= mLensRadius;
				fV *= mLensRadius;

				pRay->mOrg = Vector3(fU, fV, 0.0f);
				pRay->mDir = Math::Normalize(ptFocal - pRay->mOrg);
			}

			*pRay = Matrix::TransformRay(*pRay, mViewInv);
			pRay->mMin = float(Math::EDX_EPSILON);
			pRay->mMax = float(Math::EDX_INFINITY);
		}
	}
}