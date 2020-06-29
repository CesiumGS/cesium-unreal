#include "Cesium3DTiles/Batched3DModelContent.h"
#include <stdexcept>
#include "tiny_gltf.h"

namespace Cesium3DTiles {

	struct B3dmHeader
	{
		unsigned char magic[4];
		uint32_t version;
		uint32_t byteLength;
		uint32_t featureTableJsonByteLength;
		uint32_t featureTableBinaryByteLength;
		uint32_t batchTableJsonByteLength;
		uint32_t batchTableBinaryByteLength;
	};

	struct B3dmHeaderLegacy1
	{
		unsigned char magic[4];
		uint32_t version;
		uint32_t byteLength;
		uint32_t batchLength;
		uint32_t batchTableByteLength;
	};

	struct B3dmHeaderLegacy2
	{
		unsigned char magic[4];
		uint32_t version;
		uint32_t byteLength;
		uint32_t batchTableJsonByteLength;
		uint32_t batchTableBinaryByteLength;
		uint32_t batchLength;
	};

	Batched3DModelContent::Batched3DModelContent(const Tile& tile, const gsl::span<const uint8_t>& data) :
		TileContent(tile)
	{
		// TODO: actually use the b3dm payload
		if (data.size() < sizeof(B3dmHeader)) {
			throw std::runtime_error("The B3DM is invalid because it is too small to include a B3DM header.");
		}

		const B3dmHeader* pHeader = reinterpret_cast<const B3dmHeader*>(data.data());

		B3dmHeader header = *pHeader;
		uint32_t headerLength = sizeof(B3dmHeader);
		uint32_t batchLength;

		// Legacy header #1: [batchLength] [batchTableByteLength]
		// Legacy header #2: [batchTableJsonByteLength] [batchTableBinaryByteLength] [batchLength]
		// Current header: [featureTableJsonByteLength] [featureTableBinaryByteLength] [batchTableJsonByteLength] [batchTableBinaryByteLength]
		// If the header is in the first legacy format 'batchTableJsonByteLength' will be the start of the JSON string (a quotation mark) or the glTF magic.
		// Accordingly its first byte will be either 0x22 or 0x67, and so the minimum uint32 expected is 0x22000000 = 570425344 = 570MB. It is unlikely that the feature table JSON will exceed this length.
		// The check for the second legacy format is similar, except it checks 'batchTableBinaryByteLength' instead
		if (pHeader->batchTableJsonByteLength >= 570425344) {
			// First legacy check
			headerLength = sizeof(B3dmHeaderLegacy1);
			const B3dmHeaderLegacy1* pLegacy1 = reinterpret_cast<const B3dmHeaderLegacy1*>(data.data());
			batchLength = pLegacy1->batchLength;
			header.batchTableJsonByteLength = pLegacy1->batchTableByteLength;
			header.batchTableBinaryByteLength = 0;
			header.featureTableJsonByteLength = 0;
			header.featureTableBinaryByteLength = 0;

			// TODO
			//Batched3DModel3DTileContent._deprecationWarning(
			//	"b3dm-legacy-header",
			//	"This b3dm header is using the legacy format [batchLength] [batchTableByteLength]. The new format is [featureTableJsonByteLength] [featureTableBinaryByteLength] [batchTableJsonByteLength] [batchTableBinaryByteLength] from https://github.com/CesiumGS/3d-tiles/tree/master/specification/TileFormats/Batched3DModel."
			//);
		}
		else if (pHeader->batchTableBinaryByteLength >= 570425344) {
			// Second legacy check
			headerLength = sizeof(B3dmHeaderLegacy2);
			const B3dmHeaderLegacy2* pLegacy2 = reinterpret_cast<const B3dmHeaderLegacy2*>(data.data());
			batchLength = pLegacy2->batchLength;
			header.batchTableJsonByteLength = pLegacy2->batchTableJsonByteLength;
			header.batchTableBinaryByteLength = pLegacy2->batchTableBinaryByteLength;
			header.featureTableJsonByteLength = 0;
			header.featureTableBinaryByteLength = 0;

			// TODO
			//Batched3DModel3DTileContent._deprecationWarning(
			//	"b3dm-legacy-header",
			//	"This b3dm header is using the legacy format [batchTableJsonByteLength] [batchTableBinaryByteLength] [batchLength]. The new format is [featureTableJsonByteLength] [featureTableBinaryByteLength] [batchTableJsonByteLength] [batchTableBinaryByteLength] from https://github.com/CesiumGS/3d-tiles/tree/master/specification/TileFormats/Batched3DModel."
			//);
		}

		if (static_cast<uint32_t>(data.size()) < pHeader->byteLength) {
			throw std::runtime_error("The B3DM is invalid because the total data available is less than the size specified in its header.");
		}

		uint32_t glbStart =
			headerLength +
			header.featureTableJsonByteLength +
			header.featureTableBinaryByteLength +
			header.batchTableJsonByteLength +
			header.batchTableBinaryByteLength;
		uint32_t glbEnd = header.byteLength;

		if (glbEnd <= glbStart) {
			throw std::runtime_error("The B3DM is invalid because the start of the glTF model is after the end of the entire B3DM.");
		}

		gsl::span<const uint8_t> glbData = data.subspan(glbStart, glbEnd - glbStart);

		tinygltf::TinyGLTF loader;
		std::string errors;
		std::string warnings;

		bool loadSucceeded = loader.LoadBinaryFromMemory(&this->_gltf, &errors, &warnings, glbData.data(), glbData.size());
		if (!loadSucceeded) {
			throw std::runtime_error("Failed to load glTF model from B3DM.");
		}
	}

}
