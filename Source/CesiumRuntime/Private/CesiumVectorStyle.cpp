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

CesiumVectorData::ColorStyle
fillStyleToNative(const FCesiumVectorPolygonFillStyle& fillStyle) {
  return CesiumVectorData::ColorStyle{
      CesiumUtility::Color{
          fillStyle.Color.R,
          fillStyle.Color.G,
          fillStyle.Color.B,
          fillStyle.Color.A},
      (CesiumVectorData::ColorMode)fillStyle.ColorMode};
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

  return CesiumVectorData::VectorStyle{
      lineStyleToNative(this->LineStyle),
      CesiumVectorData::PolygonStyle{
          this->PolygonStyle.Fill
              ? std::optional<CesiumVectorData::ColorStyle>(
                    fillStyleToNative(this->PolygonStyle.FillStyle))
              : std::nullopt,
          this->PolygonStyle.Outline
              ? std::optional<CesiumVectorData::LineStyle>(
                    lineStyleToNative(this->PolygonStyle.OutlineStyle))
              : std::nullopt},
      CesiumVectorData::PointStyle{
          this->PointStyle.Radius,
          this->PointStyle.Fill
              ? std::optional<CesiumVectorData::ColorStyle>(
                    fillStyleToNative(this->PointStyle.FillStyle))
              : std::nullopt,
          this->PointStyle.Outline
              ? std::optional<CesiumVectorData::LineStyle>(
                    lineStyleToNative(this->PointStyle.OutlineStyle))
              : std::nullopt}};
}

FCesiumVectorStyle
FCesiumVectorStyle::fromNative(const CesiumVectorData::VectorStyle& style) {
  FCesiumVectorLineStyle PolygonOutlineStyle;
  if (style.polygon.outline) {
    PolygonOutlineStyle = FCesiumVectorLineStyle{
        FColor(
            (uint8)style.polygon.outline->color.r,
            (uint8)style.polygon.outline->color.g,
            (uint8)style.polygon.outline->color.b,
            (uint8)style.polygon.outline->color.a),
        (ECesiumVectorColorMode)style.polygon.outline->colorMode,
        style.polygon.outline->width,
        (ECesiumVectorLineWidthMode)style.polygon.outline->widthMode};
  }

  FCesiumVectorPolygonFillStyle PolygonFillStyle;
  if (style.polygon.fill) {
    PolygonFillStyle = FCesiumVectorPolygonFillStyle{
        FColor(
            (uint8)style.polygon.fill->color.r,
            (uint8)style.polygon.fill->color.g,
            (uint8)style.polygon.fill->color.b,
            (uint8)style.polygon.fill->color.a),
        (ECesiumVectorColorMode)style.polygon.fill->colorMode};
  }

  FCesiumVectorLineStyle PointOutlineStyle;
  if (style.point.outline) {
    PolygonOutlineStyle = FCesiumVectorLineStyle{
        FColor(
            (uint8)style.point.outline->color.r,
            (uint8)style.point.outline->color.g,
            (uint8)style.point.outline->color.b,
            (uint8)style.point.outline->color.a),
        (ECesiumVectorColorMode)style.point.outline->colorMode,
        style.point.outline->width,
        (ECesiumVectorLineWidthMode)style.point.outline->widthMode};
  }

  FCesiumVectorPolygonFillStyle PointFillStyle;
  if (style.point.fill) {
    PolygonFillStyle = FCesiumVectorPolygonFillStyle{
        FColor(
            (uint8)style.point.fill->color.r,
            (uint8)style.point.fill->color.g,
            (uint8)style.point.fill->color.b,
            (uint8)style.point.fill->color.a),
        (ECesiumVectorColorMode)style.point.fill->colorMode};
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
          PolygonFillStyle,
          style.polygon.outline.has_value(),
          PolygonOutlineStyle},
      FCesiumVectorPointStyle{
          (float)style.point.radius,
          style.point.fill.has_value(),
          PointFillStyle,
          style.point.outline.has_value(),
          PointOutlineStyle}};
}
