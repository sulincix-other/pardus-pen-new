#pragma once

#include <array>
#include <cmath>
#include <QMap>
#include <QPointF>
#include <QVector>
#include <algorithm>

namespace StrokeRecognitionConstants {
constexpr int RESAMPLE_POINTS = 64;
constexpr float MIN_CHANGE_DEGREE = 12.0f;
constexpr float STRONG_REGION_TURN = 60.0f;
constexpr float ANGLE_THRESHOLD = 45.0f;
constexpr float MIN_SCORE = 75.0f;
constexpr int TOTAL_GROUP = 8;
constexpr float ERROR_SCALE = 2.0f;
constexpr float CIRCLE_ANGLE_THRESHOLD = 60.0f;
constexpr float NOISE_TRESHOLD = 400.0f;
}

inline constexpr int RESAMPLE_POINTS = StrokeRecognitionConstants::RESAMPLE_POINTS;
inline constexpr float MIN_CHANGE_DEGREE = StrokeRecognitionConstants::MIN_CHANGE_DEGREE;
inline constexpr float STRONG_REGION_TURN = StrokeRecognitionConstants::STRONG_REGION_TURN;
inline constexpr float ANGLE_THRESHOLD = StrokeRecognitionConstants::ANGLE_THRESHOLD;
inline constexpr float MIN_SCORE = StrokeRecognitionConstants::MIN_SCORE;
inline constexpr int TOTAL_GROUP = StrokeRecognitionConstants::TOTAL_GROUP;
inline constexpr float ERROR_SCALE = StrokeRecognitionConstants::ERROR_SCALE;
inline constexpr float CIRCLE_ANGLE_THRESHOLD = StrokeRecognitionConstants::CIRCLE_ANGLE_THRESHOLD;
inline constexpr float NOISE_TRESHOLD = StrokeRecognitionConstants::NOISE_TRESHOLD;

struct StrokeVariables {
    std::array<QPointF, RESAMPLE_POINTS> points{};
    std::array<float, RESAMPLE_POINTS - 1> theta{};
    std::array<float, RESAMPLE_POINTS - 2> deltaTheta{};
    std::array<int, RESAMPLE_POINTS - 2> turnRegionStart{};
    std::array<int, RESAMPLE_POINTS - 2> turnRegionEnd{};
    std::array<float, RESAMPLE_POINTS - 2> turnRegionSum{};

    int pointCount = 0;
    int turnRegionCount = 0;
};

struct StrokeFeatures {
    float totalTurnDegree = 0.0f;
    float totalAbsTurnDegree = 0.0f;
    float straightnessScore = 0.0f;
    float totalLength = 0.0f;
    float noise = 0.0f;
    int directionChangeCount = 0;
    int turnRegionCount = 0;
};

struct StrokeResult {
    std::array<QPointF, 4> idealCorners{};
    std::array<float, 4> idealEdgeTheta{};
    QPointF circleCenter{};
    float circleRadius = 0.0f;
};

struct StrokeScore {
    float closureScore = 0.0f;
    float lineScore = 0.0f;
    float triangleScore = 0.0f;
    float squareScore = 0.0f;
    float circleScore = 0.0f;

    float circleRadiusScore = 0.0f;
    float squareShapeFit = 0.0f;
    float triangleShapeFit = 0.0f;

    int decision = 0;
};

int stroke_recognition(const QMap<qint64, QPointF> &points, StrokeVariables &variables,
                       StrokeResult &result);

#define RECOG_UNKNOWN 0
#define RECOG_LINE 1
#define RECOG_CIRCLE 2
#define RECOG_TRIANGLE 3
#define RECOG_SQUARE 4
#define RECOG_DISTANCE_ERROR 6
#define RECOG_LENGTH_ERROR 7
#define RECOG_START_ERROR 8