//
// Created by daozhi on 19-4-4.
//

#include <opencv2/opencv.hpp>
#include "PatchGenerator.h"

namespace cv
{

/*
  The code below implements keypoint detector, fern-based point classifier and a planar object detector.
  References:
   1. Mustafa Özuysal, Michael Calonder, Vincent Lepetit, Pascal Fua,
      "Fast KeyPoint Recognition Using Random Ferns,"
      IEEE Transactions on Pattern Analysis and Machine Intelligence, 15 Jan. 2009.
   2. Vincent Lepetit, Pascal Fua,
      "Towards Recognizing Feature Points Using Classification Trees,"
      Technical Report IC/2004/74, EPFL, 2004.
*/

    const int progressBarSize = 50;

//////////////////////////// Patch Generator //////////////////////////////////

    static const double DEFAULT_BACKGROUND_MIN = 0;
    static const double DEFAULT_BACKGROUND_MAX = 256;
    static const double DEFAULT_NOISE_RANGE = 5;
    static const double DEFAULT_LAMBDA_MIN = 0.6;
    static const double DEFAULT_LAMBDA_MAX = 1.5;
    static const double DEFAULT_THETA_MIN = -CV_PI;
    static const double DEFAULT_THETA_MAX = CV_PI;
    static const double DEFAULT_PHI_MIN = -CV_PI;
    static const double DEFAULT_PHI_MAX = CV_PI;

    PatchGenerator::PatchGenerator()
            : backgroundMin(DEFAULT_BACKGROUND_MIN), backgroundMax(DEFAULT_BACKGROUND_MAX),
              noiseRange(DEFAULT_NOISE_RANGE), randomBlur(true), lambdaMin(DEFAULT_LAMBDA_MIN),
              lambdaMax(DEFAULT_LAMBDA_MAX), thetaMin(DEFAULT_THETA_MIN),
              thetaMax(DEFAULT_THETA_MAX), phiMin(DEFAULT_PHI_MIN),
              phiMax(DEFAULT_PHI_MAX)
    {
    }


    PatchGenerator::PatchGenerator(double _backgroundMin, double _backgroundMax,
                                   double _noiseRange, bool _randomBlur,
                                   double _lambdaMin, double _lambdaMax,
                                   double _thetaMin, double _thetaMax,
                                   double _phiMin, double _phiMax )
            : backgroundMin(_backgroundMin), backgroundMax(_backgroundMax),
              noiseRange(_noiseRange), randomBlur(_randomBlur),
              lambdaMin(_lambdaMin), lambdaMax(_lambdaMax),
              thetaMin(_thetaMin), thetaMax(_thetaMax),
              phiMin(_phiMin), phiMax(_phiMax)
    {
    }


    void PatchGenerator::generateRandomTransform(Point2f srcCenter, Point2f dstCenter,
                                                 Mat& transform, RNG& rng, bool inverse) const
    {
        double lambda1 = rng.uniform(lambdaMin, lambdaMax);
        double lambda2 = rng.uniform(lambdaMin, lambdaMax);
        double theta = rng.uniform(thetaMin, thetaMax);
        double phi = rng.uniform(phiMin, phiMax);

        // Calculate random parameterized affine transformation A,
        // A = T(patch center) * R(theta) * R(phi)' *
        //     S(lambda1, lambda2) * R(phi) * T(-pt)
        double st = sin(theta);
        double ct = cos(theta);
        double sp = sin(phi);
        double cp = cos(phi);
        double c2p = cp*cp;
        double s2p = sp*sp;

        double A = lambda1*c2p + lambda2*s2p;
        double B = (lambda2 - lambda1)*sp*cp;
        double C = lambda1*s2p + lambda2*c2p;

        double Ax_plus_By = A*srcCenter.x + B*srcCenter.y;
        double Bx_plus_Cy = B*srcCenter.x + C*srcCenter.y;

        transform.create(2, 3, CV_64F);
        Mat_<double>& T = (Mat_<double>&)transform;
        T(0,0) = A*ct - B*st;
        T(0,1) = B*ct - C*st;
        T(0,2) = -ct*Ax_plus_By + st*Bx_plus_Cy + dstCenter.x;
        T(1,0) = A*st + B*ct;
        T(1,1) = B*st + C*ct;
        T(1,2) = -st*Ax_plus_By - ct*Bx_plus_Cy + dstCenter.y;

        if( inverse )
            invertAffineTransform(T, T);
    }


    void PatchGenerator::operator ()(const Mat& image, Point2f pt, Mat& patch, Size patchSize, RNG& rng) const
    {
        double buffer[6];
        Mat_<double> T(2, 3, buffer);

        generateRandomTransform(pt, Point2f((patchSize.width-1)*0.5f, (patchSize.height-1)*0.5f), T, rng);
        (*this)(image, T, patch, patchSize, rng);
    }


    void PatchGenerator::operator ()(const Mat& image, const Mat& T,
                                     Mat& patch, Size patchSize, RNG& rng) const
    {
        patch.create( patchSize, image.type() );
        if( backgroundMin != backgroundMax )
        {
            rng.fill(patch, RNG::UNIFORM, Scalar::all(backgroundMin), Scalar::all(backgroundMax));
            warpAffine(image, patch, T, patchSize, INTER_LINEAR, BORDER_TRANSPARENT);
        }
        else
            warpAffine(image, patch, T, patchSize, INTER_LINEAR, BORDER_CONSTANT, Scalar::all(backgroundMin));

        int ksize = randomBlur ? (unsigned)rng % 9 - 5 : 0;
        if( ksize > 0 )
        {
            ksize = ksize*2 + 1;
            GaussianBlur(patch, patch, Size(ksize, ksize), 0, 0);
        }

        if( noiseRange > 0 )
        {
            AutoBuffer<uchar> _noiseBuf( patchSize.width*patchSize.height*image.elemSize() );
            Mat noise(patchSize, image.type(), (uchar*)_noiseBuf);
            int delta = image.depth() == CV_8U ? 128 : image.depth() == CV_16U ? 32768 : 0;
            rng.fill(noise, RNG::NORMAL, Scalar::all(delta), Scalar::all(noiseRange));
            if( backgroundMin != backgroundMax )
                addWeighted(patch, 1, noise, 1, -delta, patch);
            else
            {
                for( int i = 0; i < patchSize.height; i++ )
                {
                    uchar* prow = patch.ptr<uchar>(i);
                    const uchar* nrow =  noise.ptr<uchar>(i);
                    for( int j = 0; j < patchSize.width; j++ )
                        if( prow[j] != backgroundMin )
                            prow[j] = saturate_cast<uchar>(prow[j] + nrow[j] - delta);
                }
            }
        }
    }

    void PatchGenerator::warpWholeImage(const Mat& image, Mat& matT, Mat& buf,
                                        Mat& warped, int border, RNG& rng) const
    {
        Mat_<double> T = matT;
        Rect roi(INT_MAX, INT_MAX, INT_MIN, INT_MIN);

        for( int k = 0; k < 4; k++ )
        {
            Point2f pt0, pt1;
            pt0.x = (float)(k == 0 || k == 3 ? 0 : image.cols);
            pt0.y = (float)(k < 2 ? 0 : image.rows);
            pt1.x = (float)(T(0,0)*pt0.x + T(0,1)*pt0.y + T(0,2));
            pt1.y = (float)(T(1,0)*pt0.x + T(1,1)*pt0.y + T(1,2));

            roi.x = std::min(roi.x, cvFloor(pt1.x));
            roi.y = std::min(roi.y, cvFloor(pt1.y));
            roi.width = std::max(roi.width, cvCeil(pt1.x));
            roi.height = std::max(roi.height, cvCeil(pt1.y));
        }

        roi.width -= roi.x - 1;
        roi.height -= roi.y - 1;
        int dx = border - roi.x;
        int dy = border - roi.y;

        if( (roi.width+border*2)*(roi.height+border*2) > buf.cols )
            buf.create(1, (roi.width+border*2)*(roi.height+border*2), image.type());

        warped = Mat(roi.height + border*2, roi.width + border*2,
                     image.type(), buf.data);

        T(0,2) += dx;
        T(1,2) += dy;
        (*this)(image, T, warped, warped.size(), rng);

        if( T.data != matT.data )
            T.convertTo(matT, matT.type());
    }


// Params are assumed to be symmetrical: lambda w.r.t. 1, theta and phi w.r.t. 0
    void PatchGenerator::setAffineParam(double lambda, double theta, double phi)
    {
        lambdaMin = 1. - lambda;
        lambdaMax = 1. + lambda;
        thetaMin = -theta;
        thetaMax = theta;
        phiMin = -phi;
        phiMax = phi;
    }
};