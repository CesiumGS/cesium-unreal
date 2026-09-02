#include "CesiumVectorStyle.h"

#include "CesiumUtility/Color.h"

namespace {
constexpr void checkEnumsMatch() {
  static_assert(
      static_cast<uint8>(CesiumVectorData::ColorMode::Normal) ==
      static_cast<uint8>(ECesiumVectorColorMode::Normal));
  static_assert(
      static_cast<uint8>(CesiumVectorData::ColorMode::Random) ==
      static_cast<uint8>(ECesiumVectorColorMode::Random));
  static_assert(
      static_cast<uint8>(CesiumVectorData::LineWidthMode::Meters) ==
      static_cast<uint8>(ECesiumVectorLineWidthMode::Meters));
  static_assert(
      static_cast<uint8>(CesiumVectorData::LineWidthMode::Pixels) ==
      static_cast<uint8>(ECesiumVectorLineWidthMode::Pixels));
}
} // namespace

CesiumVectorData::LineStyle FCesiumVectorLineStyle::toNative() const {
  checkEnumsMatch();

  return CesiumVectorData::LineStyle{
      {CesiumUtility::Color{
           this->Color.R,
           this->Color.G,
           this->Color.B,
           this->Color.A},
       (CesiumVectorData::ColorMode)this->ColorMode},
      this->Width,
      (CesiumVectorData::LineWidthMode)this->WidthMode};
}

FCesiumVectorLineStyle
FCesiumVectorLineStyle::fromNative(const CesiumVectorData::LineStyle& style) {
  checkEnumsMatch();

  return FCesiumVectorLineStyle{
      FColor(
          static_cast<uint8>(style.color.r),
          static_cast<uint8>(style.color.g),
          static_cast<uint8>(style.color.b),
          static_cast<uint8>(style.color.a)),
      (ECesiumVectorColorMode)style.colorMode,
      style.width,
      (ECesiumVectorLineWidthMode)style.widthMode};
}

CesiumVectorData::ColorStyle FCesiumVectorPolygonFillStyle::toNative() const {
  checkEnumsMatch();

  return CesiumVectorData::ColorStyle{
      CesiumUtility::Color{
          this->Color.R,
          this->Color.G,
          this->Color.B,
          this->Color.A},
      (CesiumVectorData::ColorMode)this->ColorMode};
}

FCesiumVectorPolygonFillStyle FCesiumVectorPolygonFillStyle::fromNative(
    const CesiumVectorData::ColorStyle& style) {
  checkEnumsMatch();

  return FCesiumVectorPolygonFillStyle{
      FColor(
          static_cast<uint8>(style.color.r),
          static_cast<uint8>(style.color.g),
          static_cast<uint8>(style.color.b),
          static_cast<uint8>(style.color.a)),
      (ECesiumVectorColorMode)style.colorMode};
}

CesiumVectorData::PolygonStyle FCesiumVectorPolygonStyle::toNative() const {
  return CesiumVectorData::PolygonStyle{
      this->Fill ? std::optional<CesiumVectorData::ColorStyle>(
                       this->FillStyle.toNative())
                 : std::nullopt,
      this->Outline ? std::optional<CesiumVectorData::LineStyle>(
                          this->OutlineStyle.toNative())
                    : std::nullopt};
}

FCesiumVectorPolygonStyle FCesiumVectorPolygonStyle::fromNative(
    const CesiumVectorData::PolygonStyle& style) {
  FCesiumVectorLineStyle OutlineStyle =
      style.outline ? FCesiumVectorLineStyle::fromNative(*style.outline)
                    : FCesiumVectorLineStyle{};
  FCesiumVectorPolygonFillStyle FillStyle =
      style.fill ? FCesiumVectorPolygonFillStyle::fromNative(*style.fill)
                 : FCesiumVectorPolygonFillStyle{};
  return FCesiumVectorPolygonStyle{
      style.fill.has_value(),
      FillStyle,
      style.outline.has_value(),
      OutlineStyle};
}

CesiumVectorData::PointStyle FCesiumVectorPointStyle::toNative() const {
  return CesiumVectorData::PointStyle{
      this->Radius,
      this->Fill ? std::optional<CesiumVectorData::ColorStyle>(
                       this->FillStyle.toNative())
                 : std::nullopt,
      this->Outline ? std::optional<CesiumVectorData::LineStyle>(
                          this->OutlineStyle.toNative())
                    : std::nullopt};
}

FCesiumVectorPointStyle
FCesiumVectorPointStyle::fromNative(const CesiumVectorData::PointStyle& style) {
  FCesiumVectorLineStyle OutlineStyle =
      style.outline ? FCesiumVectorLineStyle::fromNative(*style.outline)
                    : FCesiumVectorLineStyle{};
  FCesiumVectorPolygonFillStyle FillStyle =
      style.fill ? FCesiumVectorPolygonFillStyle::fromNative(*style.fill)
                 : FCesiumVectorPolygonFillStyle{};
  return FCesiumVectorPointStyle{
      (float)style.radius,
      style.fill.has_value(),
      FillStyle,
      style.outline.has_value(),
      OutlineStyle};
}

CesiumVectorData::VectorStyle FCesiumVectorStyle::toNative() const {
  return CesiumVectorData::VectorStyle{
      this->LineStyle.toNative(),
      this->PolygonStyle.toNative(),
      this->PointStyle.toNative()};
}

FCesiumVectorStyle
FCesiumVectorStyle::fromNative(const CesiumVectorData::VectorStyle& style) {
  return FCesiumVectorStyle{
      FCesiumVectorLineStyle::fromNative(style.line),
      FCesiumVectorPolygonStyle::fromNative(style.polygon),
      FCesiumVectorPointStyle::fromNative(style.point)};
}
