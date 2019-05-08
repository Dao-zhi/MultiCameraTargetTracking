//
// Created by daozhi on 19-4-4.
//
#include <opencv2/opencv.hpp>

#ifndef PATCHGENERATOR_H
#define PATCHGENERATOR_H

namespace cv
{
    class CV_EXPORTS PatchGenerator
    {
    public:
        PatchGenerator();
        PatchGenerator(double _backgroundMin, double _backgroundMax,
                       double _noiseRange, bool _randomBlur=true,
                       double _lambdaMin=0.6, double _lambdaMax=1.5,
                       double _thetaMin=-CV_PI, double _thetaMax=CV_PI,
                       double _phiMin=-CV_PI, double _phiMax=CV_PI );
        void operator()(const Mat& image, Point2f pt, Mat& patch, Size patchSize, RNG& rng) const;
        void operator()(const Mat& image, const Mat& transform, Mat& patch,
                        Size patchSize, RNG& rng) const;
        void warpWholeImage(const Mat& image, Mat& matT, Mat& buf,
                            CV_OUT Mat& warped, int border, RNG& rng) const;
        void generateRandomTransform(Point2f srcCenter, Point2f dstCenter,
                                     CV_OUT Mat& transform, RNG& rng,
                                     bool inverse=false) const;
        void setAffineParam(double lambda, double theta, double phi);

        double backgroundMin, backgroundMax;
        double noiseRange;
        bool randomBlur;
        double lambdaMin, lambdaMax;
        double thetaMin, thetaMax;
        double phiMin, phiMax;
    };
};
#endif