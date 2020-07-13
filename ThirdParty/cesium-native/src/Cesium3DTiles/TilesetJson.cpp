#include "TilesetJson.h"

namespace Cesium3DTiles {

	std::optional<BoundingVolume> TilesetJson::getBoundingVolumeProperty(const nlohmann::json& tileJson, const std::string& key) {
		using nlohmann::json;

		json::const_iterator bvIt = tileJson.find(key);
		if (bvIt == tileJson.end()) {
			return std::optional<BoundingVolume>();
		}

		json::const_iterator boxIt = bvIt->find("box");
		if (boxIt != bvIt->end() && boxIt->is_array() && boxIt->size() >= 12) {
			const json& a = *boxIt;
			return BoundingBox(
				glm::dvec3(a[0], a[1], a[2]),
				glm::dmat3(a[3], a[4], a[5], a[6], a[7], a[8], a[9], a[10], a[11])
			);
		}

		json::const_iterator regionIt = bvIt->find("region");
		if (regionIt != bvIt->end() && regionIt->is_array() && regionIt->size() >= 6) {
			const json& a = *regionIt;
			return BoundingRegion(a[0], a[1], a[2], a[3], a[4], a[5]);
		}

		json::const_iterator sphereIt = bvIt->find("sphere");
		if (sphereIt != bvIt->end() && sphereIt->is_array() && sphereIt->size() >= 4) {
			const json& a = *sphereIt;
			return BoundingSphere(glm::dvec3(a[0], a[1], a[2]), a[3]);
		}

		return std::optional<BoundingVolume>();
	}

	std::optional<double> TilesetJson::getScalarProperty(const nlohmann::json& tileJson, const std::string& key) {
		using nlohmann::json;

		json::const_iterator it = tileJson.find(key);
		if (it == tileJson.end() || !it->is_number()) {
			return std::optional<double>();
		}

		return it->get<double>();
	}

	std::optional<glm::dmat4x4> TilesetJson::getTransformProperty(const nlohmann::json& tileJson, const std::string& key) {
		using nlohmann::json;

		json::const_iterator it = tileJson.find(key);
		if (it == tileJson.end() || !it->is_array() || it->size() < 16) {
			return std::optional<glm::dmat4x4>();
		}

		const json& a = *it;
		return glm::dmat4(
			glm::dvec4(a[0], a[1], a[2], a[3]),
			glm::dvec4(a[4], a[5], a[6], a[7]),
			glm::dvec4(a[8], a[9], a[10], a[11]),
			glm::dvec4(a[12], a[13], a[14], a[15])
		);
	}

}
