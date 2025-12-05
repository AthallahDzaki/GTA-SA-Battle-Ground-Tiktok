#pragma once

#include <cmath>
#include <random>
#include <cstdint>

namespace BattleGround {

class Math {
public:
    static constexpr float PI = 3.14159265358979323846f;
    static constexpr float DEG_TO_RAD = PI / 180.0f;
    static constexpr float RAD_TO_DEG = 180.0f / PI;

    // Random number generation
    static float RandomFloat(float min, float max) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dis(min, max);
        return dis(gen);
    }

    static int RandomInt(int min, int max) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<int> dis(min, max);
        return dis(gen);
    }

    // Vector operations (compatible with CVector)
    struct Vector3 {
        float x, y, z;

        Vector3() : x(0), y(0), z(0) {}
        Vector3(float x, float y, float z) : x(x), y(y), z(z) {}

        Vector3 operator+(const Vector3& other) const {
            return Vector3(x + other.x, y + other.y, z + other.z);
        }

        Vector3 operator-(const Vector3& other) const {
            return Vector3(x - other.x, y - other.y, z - other.z);
        }

        Vector3 operator*(float scalar) const {
            return Vector3(x * scalar, y * scalar, z * scalar);
        }

        float Length() const {
            return std::sqrt(x * x + y * y + z * z);
        }

        float LengthSquared() const {
            return x * x + y * y + z * z;
        }

        Vector3 Normalized() const {
            float len = Length();
            if (len > 0.0001f) {
                return Vector3(x / len, y / len, z / len);
            }
            return Vector3(0, 0, 0);
        }

        float Dot(const Vector3& other) const {
            return x * other.x + y * other.y + z * other.z;
        }

        Vector3 Cross(const Vector3& other) const {
            return Vector3(
                y * other.z - z * other.y,
                z * other.x - x * other.z,
                x * other.y - y * other.x
            );
        }
    };

    // Generate random point in circle
    static Vector3 RandomPointInCircle(const Vector3& center, float radius) {
        float angle = RandomFloat(0, 2.0f * PI);
        float r = std::sqrt(RandomFloat(0, 1)) * radius;
        return Vector3(
            center.x + r * std::cos(angle),
            center.y + r * std::sin(angle),
            center.z
        );
    }

    // Linear interpolation
    static float Lerp(float a, float b, float t) {
        return a + (b - a) * Clamp01(t);
    }

    static Vector3 Lerp(const Vector3& a, const Vector3& b, float t) {
        return Vector3(
            Lerp(a.x, b.x, t),
            Lerp(a.y, b.y, t),
            Lerp(a.z, b.z, t)
        );
    }

    // Clamp functions
    static float Clamp(float value, float min, float max) {
        if (value < min) return min;
        if (value > max) return max;
        return value;
    }

    static float Clamp01(float value) {
        return Clamp(value, 0.0f, 1.0f);
    }

    // Distance calculation
    static float Distance(const Vector3& a, const Vector3& b) {
        return (b - a).Length();
    }

    static float DistanceSquared(const Vector3& a, const Vector3& b) {
        return (b - a).LengthSquared();
    }
};

} // namespace BattleGround
