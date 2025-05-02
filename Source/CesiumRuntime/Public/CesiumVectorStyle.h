#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "CesiumVectorData/VectorStyle.h"

#include "CesiumVectorStyle.generated.h"

/**
 * The mode used to render polylines and strokes.
 */
 UENUM(BlueprintType)
 enum class ECesiumVectorLineWidthMode : uint8 {
   /**
    * The line width represents the number of pixels the line will take up,
    * regardless of LOD.
    */
   Pixels = 0,
   /**
    * The line width represents the physical size of the line in meters.
    */
   Meters = 1
 };
 
 /**
  * The mode used to interpret the color value provided in a style.
  */
 UENUM(BlueprintType)
 enum class ECesiumVectorColorMode : uint8 {
   /**
    * The normal color mode. The color will be used directly.
    */
   Normal = 0,
   /**
    * The color will be chosen randomly.
    *
    * The color randomization will be applied to each component, with the
    * resulting value between 0 and the specified color component value. Alpha is
    * always ignored. For example, if the color was 0xff000077 (only 0x77 in the
    * green component), the resulting randomized value could be 0xff000041, or
    * 0xff000076, but never 0xff0000aa.
    */
   Random = 1
 };
 
 /**
  * The style used to draw polylines and strokes.
  */
 USTRUCT(BlueprintType)
 struct FCesiumVectorLineStyle {
   GENERATED_BODY()
 
   /**
    * The color to be used.
    */
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
   FColor Color = FColor(1.0f, 1.0f, 1.0f, 1.0f);
   /**
    * The color mode to be used.
    */
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
   ECesiumVectorColorMode ColorMode =
       ECesiumVectorColorMode::Normal;
   /**
    * The width of the line or stroke, with the unit specified by `WidthMode`.
    */
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium", meta = (ClampMin = "0"))
   double Width = 1.0;
   /**
    * The mode to use when interpreting `Width`.
    */
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
   ECesiumVectorLineWidthMode WidthMode =
       ECesiumVectorLineWidthMode::Pixels;
 };
 
 /**
  * The style used to draw polygons.
  */
 USTRUCT(BlueprintType)
 struct FCesiumVectorPolygonStyle {
   GENERATED_BODY()
 
   /**
    * The color to be used.
    */
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
   FColor Color = FColor(1.0f, 1.0f, 1.0f, 1.0f);
   /**
    * The color mode to be used.
    */
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
   ECesiumVectorColorMode ColorMode =
       ECesiumVectorColorMode::Normal;
   /**
    * Whether the polygon should be filled. The `ColorStyle` specified
    * here will be used for the fill color.
    */
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
   bool Fill = true;
   /**
    * Whether the polygon should be outlined. The `LineStyle` specified on
    * the same `FCesiumVectorDocumentRasterOverlayStyle` as this will be used for
    * the stroke color.
    */
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
   bool Outline = false;
 };
 
 /**
  * Style information to use when drawing vector data.
  */
 USTRUCT(BlueprintType)
 struct FCesiumVectorStyle {
   GENERATED_BODY()
 
   /**
    * Styles to use when drawing polylines and stroking shapes.
    */
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
   FCesiumVectorLineStyle LineStyle;
 
   /**
    * Styles to use when drawing polygons.
    */
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
   FCesiumVectorPolygonStyle PolygonStyle;

   /**
    * Converts this Unreal representation into the Cesium Native equivalent.
    */
   CesiumVectorData::VectorStyle toNative() const;

   /**
    * Creates this Unreal representation from the Cesium Native equivalent.
    */
   static FCesiumVectorStyle fromNative(const CesiumVectorData::VectorStyle& style);
 };
