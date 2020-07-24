#pragma once

#include <glm/gtc/epsilon.hpp>
#include <algorithm>

namespace Cesium3DTiles {

    class CESIUM3DTILES_API Math {
    public:
        static const double EPSILON1;
        static const double EPSILON2;
        static const double EPSILON3;
        static const double EPSILON4;
        static const double EPSILON5;
        static const double EPSILON6;
        static const double EPSILON7;
        static const double EPSILON8;
        static const double EPSILON9;
        static const double EPSILON10;
        static const double EPSILON11;
        static const double EPSILON12;
        static const double EPSILON13;
        static const double EPSILON14;
        static const double EPSILON15;
        static const double EPSILON16;
        static const double EPSILON17;
        static const double EPSILON18;
        static const double EPSILON19;
        static const double EPSILON20;
        static const double EPSILON21;

        static const double PI;
        static const double TWO_PI;
        static const double PI_OVER_TWO;

        template<glm::length_t L, typename T, glm::qualifier Q>
        static inline glm::vec<L, T, Q> relativeEpsilonToAbsolute(const glm::vec<L, T, Q>& a, const glm::vec<L, T, Q>& b, double relativeEpsilon) {
            return relativeEpsilon * glm::max(glm::abs(a), glm::abs(b));
        }

        static inline double relativeEpsilonToAbsolute(double a, double b, double relativeEpsilon) {
            return relativeEpsilon * std::max(std::abs(a), std::abs(b));
        }

        template<glm::length_t L, typename T, glm::qualifier Q>
        static bool equalsEpsilon(const glm::vec<L, T, Q>& left, const glm::vec<L, T, Q>& right, double relativeEpsilon) {
            return Math::equalsEpsilon(left, right, relativeEpsilon, relativeEpsilon);
        }

        static inline bool equalsEpsilon(double left, double right, double relativeEpsilon) {
            return equalsEpsilon(left, right, relativeEpsilon, relativeEpsilon);
        }

        static inline bool equalsEpsilon(double left, double right, double relativeEpsilon, double absoluteEpsilon) {
            double diff = std::abs(left - right);
            return
                diff <= absoluteEpsilon ||
                diff <= relativeEpsilonToAbsolute(left, right, relativeEpsilon);
        }

        template<glm::length_t L, typename T, glm::qualifier Q>
        static inline bool equalsEpsilon(const glm::vec<L, T, Q>& left, const glm::vec<L, T, Q>& right, double relativeEpsilon, double absoluteEpsilon) {
            glm::vec<L, T, Q> diff = glm::abs(left - right);
            return
                glm::lessThanEqual(diff, glm::vec<L, T, Q>(absoluteEpsilon)) == glm::vec<L, bool, Q>(true) ||
                glm::lessThanEqual(diff, relativeEpsilonToAbsolute(left, right, relativeEpsilon)) == glm::vec<L, bool, Q>(true);
        }

        static inline double sign(double value) {
            if (value == 0.0 || value != value) {
                // zero or NaN
                return value;
            }
            return value > 0 ? 1 : -1;
        }

        static inline double negativePiToPi(double angle) {
            return Math::zeroToTwoPi(angle + Math::PI) - Math::PI;
        }

        static inline double zeroToTwoPi(double angle) {
            double mod = Math::mod(angle, Math::TWO_PI);
            if (
                std::abs(mod) < Math::EPSILON14 &&
                std::abs(angle) > Math::EPSILON14
            ) {
                return Math::TWO_PI;
            }
            return mod;
        }

        static inline double mod(double m, double n) {
            return fmod(fmod(m, n) + n, n);
        }
    };

}
