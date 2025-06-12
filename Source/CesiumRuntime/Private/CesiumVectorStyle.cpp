#include "CesiumVectorStyle.h"

#include "CesiumUtility/Color.h"

CesiumVectorData::VectorStyle FCesiumVectorStyle::toNative() const {
  return CesiumVectorData::VectorStyle{
      CesiumVectorData::LineStyle{
          CesiumUtility::Color{
              this->LineStyle.Color.R,
              this->LineStyle.Color.G,
              this->LineStyle.Color.B,
              this->LineStyle.Color.A},
          (CesiumVectorData::ColorMode)this->LineStyle.ColorMode,
          this->LineStyle.Width,
          (CesiumVectorData::LineWidthMode)this->LineStyle.WidthMode},
      CesiumVectorData::PolygonStyle{
          CesiumUtility::Color{
              this->PolygonStyle.Color.R,
              this->PolygonStyle.Color.G,
              this->PolygonStyle.Color.B,
              this->PolygonStyle.Color.A},
          (CesiumVectorData::ColorMode)this->PolygonStyle.ColorMode,
          this->PolygonStyle.Fill,
          this->PolygonStyle.Outline}};
}

FCesiumVectorStyle
FCesiumVectorStyle::fromNative(const CesiumVectorData::VectorStyle& style) {
  return FCesiumVectorStyle{
      FCesiumVectorLineStyle{
          FColor(
              (uint8)style.line.color.r,
              (uint8)style.line.color.g,
              (uint8)style.line.color.b,
              (uint8)style.line.color.a),
          (ECesiumVectorColorMode)style.line.colorMode,
          style.line.width,
          (ECesiumVectorLineWidthMode)style.line.widthMode},
      FCesiumVectorPolygonStyle{
          FColor(
              (uint8)style.polygon.color.r,
              (uint8)style.polygon.color.g,
              (uint8)style.polygon.color.b,
              (uint8)style.polygon.color.a),
          (ECesiumVectorColorMode)style.polygon.colorMode,
          style.polygon.fill,
          style.polygon.outline}};
}
