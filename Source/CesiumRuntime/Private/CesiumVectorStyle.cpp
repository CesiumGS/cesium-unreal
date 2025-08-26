#include "CesiumVectorStyle.h"

#include "CesiumUtility/Color.h"

namespace {
CesiumVectorData::LineStyle
lineStyleToNative(const FCesiumVectorLineStyle& InLineStyle) {
  return CesiumVectorData::LineStyle{
      {CesiumUtility::Color{
           InLineStyle.Color.R,
           InLineStyle.Color.G,
           InLineStyle.Color.B,
           InLineStyle.Color.A},
       (CesiumVectorData::ColorMode)InLineStyle.ColorMode},
      InLineStyle.Width,
      (CesiumVectorData::LineWidthMode)InLineStyle.WidthMode};
}
} // namespace

CesiumVectorData::VectorStyle FCesiumVectorStyle::toNative() const {
  // Assert that enums are equivalent to catch any issues.
  static_assert(
      (uint8)CesiumVectorData::ColorMode::Normal ==
      (uint8)ECesiumVectorColorMode::Normal);
  static_assert(
      (uint8)CesiumVectorData::ColorMode::Random ==
      (uint8)ECesiumVectorColorMode::Random);
  static_assert(
      (uint8)CesiumVectorData::LineWidthMode::Meters ==
      (uint8)ECesiumVectorLineWidthMode::Meters);
  static_assert(
      (uint8)CesiumVectorData::LineWidthMode::Pixels ==
      (uint8)ECesiumVectorLineWidthMode::Pixels);

  std::optional<CesiumVectorData::ColorStyle> fillStyle;
  if (this->PolygonStyle.Fill) {
    fillStyle = CesiumVectorData::ColorStyle{
        CesiumUtility::Color{
            this->PolygonStyle.FillStyle.Color.R,
            this->PolygonStyle.FillStyle.Color.G,
            this->PolygonStyle.FillStyle.Color.B,
            this->PolygonStyle.FillStyle.Color.A},
        (CesiumVectorData::ColorMode)this->PolygonStyle.FillStyle.ColorMode};
  }

  return CesiumVectorData::VectorStyle{
      lineStyleToNative(this->LineStyle),
      CesiumVectorData::PolygonStyle{
          fillStyle,
          this->PolygonStyle.Outline
              ? std::optional<CesiumVectorData::LineStyle>(
                    lineStyleToNative(this->PolygonStyle.OutlineStyle))
              : std::nullopt}};
}

FCesiumVectorStyle
FCesiumVectorStyle::fromNative(const CesiumVectorData::VectorStyle& style) {
  FCesiumVectorLineStyle OutlineStyle;
  if (style.polygon.outline) {
    OutlineStyle = FCesiumVectorLineStyle{
        FColor(
            (uint8)style.polygon.outline->color.r,
            (uint8)style.polygon.outline->color.g,
            (uint8)style.polygon.outline->color.b,
            (uint8)style.polygon.outline->color.a),
        (ECesiumVectorColorMode)style.polygon.outline->colorMode,
        style.polygon.outline->width,
        (ECesiumVectorLineWidthMode)style.polygon.outline->widthMode};
  }

  FCesiumVectorPolygonFillStyle FillStyle;
  if (style.polygon.fill) {
    FillStyle = FCesiumVectorPolygonFillStyle{
        FColor(
            (uint8)style.polygon.fill->color.r,
            (uint8)style.polygon.fill->color.g,
            (uint8)style.polygon.fill->color.b,
            (uint8)style.polygon.fill->color.a),
        (ECesiumVectorColorMode)style.polygon.fill->colorMode};
  }

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
          style.polygon.fill.has_value(),
          FillStyle,
          style.polygon.outline.has_value(),
          OutlineStyle}};
}
