// Copyright 2020-2024 CesiumGS, Inc. and Contributors

/*=============================================================================
	CesiumVoxelTemplate.hlsl: Template for creating custom shaders to style voxel data.
=============================================================================*/

/*=======================
  BEGIN CUSTOM SHADER
=========================*/

struct CustomShaderProperties
{
%s
};

struct CustomShader
{
%s
  
  float4 Shade(CustomShaderProperties Properties)
  {
%s
  }
};

/*=======================
  END CUSTOM SHADER
=========================*/

/*===========================
  BEGIN RAY + INTERSECTION UTILITY
=============================*/

#define CZM_INFINITY 5906376272000.0 // Distance from the Sun to Pluto in meters.
#define NO_HIT -CZM_INFINITY
#define INF_HIT (CZM_INFINITY * 0.5)

#define CZM_PI_OVER_TWO 1.5707963267948966
#define CZM_PI 3.141592653589793
#define CZM_TWO_PI 6.283185307179586

struct Ray
{
  float3 Origin;
  float3 Direction;
};

struct Intersection
{
  float t;
  float3 Normal;
};

// Represents where a ray enters and leaves a volume.
struct RayIntersections
{
  Intersection Entry;
  Intersection Exit;
};

struct RayIntersectionUtility
{
  float MaxComponent(float3 v)
  {
    return max(max(v.x, v.y), v.z);
  }

  float MinComponent(float3 v)
  {
    return min(min(v.x, v.y), v.z);
  }

  bool3 Equal(float3 v1, float3 v2)
  {
    return bool3(v1.x == v2.x, v1.y == v2.y, v1.z == v2.z);
  }

  Intersection NewMissedIntersection()
  {
    Intersection result = (Intersection) 0;
    result.t = NO_HIT;
    return result;
  }
  
  Intersection NewIntersection(float t, float3 Normal)
  {
    Intersection result = (Intersection) 0;
    result.t = t;
    result.Normal = Normal;
    return result;
  }

  RayIntersections NewRayIntersections(Intersection Entry, Intersection Exit)
  {
    RayIntersections result = (RayIntersections) 0;
    result.Entry = Entry;
    result.Exit = Exit;
    return result;
  }

  Intersection Min(in Intersection A, in Intersection B)
  {
    if (A.t <= B.t)
    {
      return A;
    }
    return B;
  }

  Intersection Max(in Intersection A, in Intersection B)
  {
    if (A.t >= B.t)
    {
      return A;
    }
    return B;
  }
  
  bool IsInRange(in float3 v, in float3 min, in float3 max)
  {
    bool3 inRange = (clamp(v, min, max) == v);
    return inRange.x && inRange.y && inRange.z;
  }
  
  /**
  * Construct an integer value (little endian) from two normalized uint8 values.
  */
  int ConstructInt(in float2 value)
  {
    return int(value.x * 255.0) + (256 * int(value.y * 255.0));
  }

  Intersection Multiply(in Intersection intersection, in float scalar)
  {
    return NewIntersection(intersection.t * scalar, intersection.Normal * scalar);
  }

  /*
   * Resolves two intersection ranges of the ray by computing their overlap.
   * For example:
   *         A   |---------|
   *         B         |-------------|
   * Ray:    O =========================>
   * Output:           |---|
   */
  RayIntersections ResolveIntersections(in RayIntersections A, in RayIntersections B)
  {
    bool missed = (A.Entry.t == NO_HIT) || (B.Entry.t == NO_HIT) ||
      (A.Exit.t < B.Entry.t) || (A.Entry.t > B.Exit.t);
  
    if (missed)
    {
      Intersection miss = NewMissedIntersection();
      return NewRayIntersections(miss, miss);
    }

    Intersection entry = Max(A.Entry, B.Entry);
    Intersection exit = Min(A.Exit, B.Exit);

    return NewRayIntersections(entry, exit);
  }
};

// SHAPE_INTERSECTIONS is the number of ray-*shape* intersections (i.e., the volume intersection pairs),
// INTERSECTIONS_LENGTH is the number of ray-*surface* intersections.
#define SHAPE_INTERSECTIONS 7
#define INTERSECTIONS_LENGTH SHAPE_INTERSECTIONS * 2

// HLSL does not like array struct members, so the data has to be stored at the top-level.
// The size is also hardcoded because dynamically sized arrays are not allowed.
float4 miss = float4(0, 0, 0, NO_HIT);
float4 IntersectionList[INTERSECTIONS_LENGTH] =
{
  miss, miss, miss, miss, miss, miss, miss,
  miss, miss, miss, miss, miss, miss, miss,
};

struct IntersectionListState
{
  // Don't access these member variables directly - call the functions instead.
  // Store an array of encoded ray-surface intersections. (See EncodeIntersection).
  
  int Length; // Set based on ShapeConstant
  int Index; // Used to emulate dynamic indexing.
  
  // The variables below relate to the shapes that the intersection is inside (counting when it is
  // on the surface itself). e.g., given a hollow ellipsoid volume,
  // count = 1 on the outer ellipsoid, 2 on the inner ellipsoid.
  int SurroundingShapeCount;
  bool IsInsidePositiveShape; // will be true as long as it is inside any positive shape.
  
  /**
  * Intersections are encoded as float4s:
  * - .xyz for the surface normal at the intersection point
  * - .w for the T value
  * The normal's scale encodes the shape intersection type: 
  * length(intersection.xyz) = 1: positive shape entry  
  * length(intersection.xyz) = 2: positive shape exit
  * length(intersection.xyz) = 3: negative shape entry  
  * length(intersection.xyz) = 4: negative shape exit
  *
  * When the voxel volume is hollow, the "positive" shape is the original volume.
  * The "negative" shape is subtracted from the positive shape.
  */
  float4 EncodeIntersection(in Intersection Input, bool IsPositive, bool IsEntering)
  {
    float scale = float(!IsPositive) * 2.0 + float(!IsEntering) + 1.0;
    return float4(Input.Normal * scale, Input.t);
  }

  /**
  * Sort the intersections from min T to max T with bubble sort. Also prepares for iteration
  * over the intersections.
  *
  * Note: If this sorting function changes, some of the intersection tests may need to be updated.
  * Search for "Sort()" to find those areas.
  */
  void Sort(inout float4 Data[INTERSECTIONS_LENGTH])
  {
    const int sortPasses = INTERSECTIONS_LENGTH - 1;
    for (int n = sortPasses; n > 0; --n)
    {
      // Skip to n = Length - 1
      if (n >= Length)
      {
        continue;
      }
      
      for (int i = 0; i < sortPasses; ++i)
      {
      // The loop should be: for (i = 0; i < n; ++i) {...} but since loops with
      // non-constant conditions are not allowed, this breaks early instead.
        if (i >= n)
        {
          break;
        }

        float4 first = Data[i + 0];
        float4 second = Data[i + 1];

        bool inOrder = first.w <= second.w;
        Data[i + 0] = inOrder ? first : second;
        Data[i + 1] = inOrder ? second : first;
      }
    }

    // Prepare initial state for GetNextIntersections()
    Index = 0;
    SurroundingShapeCount = 0;
    IsInsidePositiveShape = false;
  }

  RayIntersections GetFirstIntersections(in float4 Data[INTERSECTIONS_LENGTH])
  {
    RayIntersections result = (RayIntersections) 0;
    result.Entry.t = Data[0].w;
    result.Entry.Normal = normalize(Data[0].xyz);
    result.Exit.t = Data[1].w;
    result.Exit.Normal = normalize(Data[1].xyz);

    return result;
  }

  /**
  * Gets the intersection at the current value of Index, while managing the state of the ray's
  * trajectory with respect to the intersected shapes.
  */
  RayIntersections GetNextIntersections(in float4 Data[INTERSECTIONS_LENGTH])
  {
    RayIntersections result = (RayIntersections) 0;
    result.Entry.t = NO_HIT;
    result.Exit.t = NO_HIT;

    if (Index >= Length)
    {
      return result;
    }

    float4 surfaceIntersection = float4(0, 0, 0, NO_HIT);

    for (int i = 0; i < INTERSECTIONS_LENGTH; ++i)
    {
      // The loop should be: for (i = index; i < loopCount; ++i) {...} but it's not possible
      // to loop with non-constant condition. Instead, continue until i = index.
      if (i < Index)
      {
        continue;
      }

      Index = i + 1;

      surfaceIntersection = Data[i];
      // Maps from [1-4] -> [0-3] (see EncodeIntersection for the types)
      int intersectionType = int(length(surfaceIntersection.xyz) - 0.5);
      bool isCurrentShapePositive = intersectionType < 2;
      bool isEnteringShape = (intersectionType % 2) == 0;

      SurroundingShapeCount += isEnteringShape ? +1 : -1;
      IsInsidePositiveShape = isCurrentShapePositive ? isEnteringShape : IsInsidePositiveShape;

      // True if entering positive shape or exiting negative shape
      if (IsInsidePositiveShape && isEnteringShape == isCurrentShapePositive)
      {
        result.Entry.t = surfaceIntersection.w;
        result.Entry.Normal = normalize(surfaceIntersection.xyz);
      }

      // True if exiting the outermost positive shape
      bool isExitingOutermostShape = !isEnteringShape && isCurrentShapePositive && SurroundingShapeCount == 0;
      // True if entering negative shape while being inside a positive one
      bool isEnteringNegativeFromPositive = isEnteringShape && !isCurrentShapePositive && SurroundingShapeCount == 2 && IsInsidePositiveShape;
      
      if (isExitingOutermostShape || isEnteringNegativeFromPositive)
      {
        result.Exit.t = surfaceIntersection.w;
        result.Exit.Normal = normalize(surfaceIntersection.xyz);
        // Entry and exit have been found, so the loop can stop
        if (isExitingOutermostShape)
        {
          // After exiting the outermost positive shape, there is nothing left to intersect. Jump to the end.
          Index = INTERSECTIONS_LENGTH;
        }
        break;
      }
      // Otherwise, keep searching for the correct exit.
    }
    
    return result;
  }
};

// Use defines instead of real functions to get array access with a non-constant index.

/**
* Encodes and stores a single intersection.
*/
#define setSurfaceIntersection(/*inout float4[]*/ list, /*in IntersectionListState*/ state, /*int*/ index, /*Intersection*/ intersection, /*bool*/ isPositive, /*bool*/ isEntering) (list)[(index)] = (state).EncodeIntersection((intersection), (isPositive), (isEntering))

/**
* Encodes and stores the given shape intersections, i.e., the intersections where a ray enters and exits a volume.
*/
#define setShapeIntersections(/*inout float4[]*/ list, /*in IntersectionListState*/ state, /*int*/ pairIndex, /*RayIntersection*/ intersections) (list)[(pairIndex) * 2 + 0] = (state).EncodeIntersection((intersections).Entry, (pairIndex) == 0, true); (list)[(pairIndex) * 2 + 1] = (state).EncodeIntersection((intersections).Exit, (pairIndex) == 0, false)

/*===========================
  END RAY + INTERSECTION UTILITY
=============================*/

/*===========================
  BEGIN SHAPE UTILITY
=============================*/

#define BOX 1
#define CYLINDER 2
#define ELLIPSOID 3

struct ShapeUtility
{
  RayIntersectionUtility Utils;
  IntersectionListState ListState;
  
  int ShapeConstant;

  float3 MinBounds;
  float3 MaxBounds;

  // Ellipsoid variables
  float3 RadiiUV;
  float3 InverseRadiiUVSquared;
  float InverseHeightDifferenceUV;

  float EccentricitySquared;
  float2 EvoluteScale;
  float3 LongitudeUVExtents; // X = min, Y = max, Z = midpoint
  float LongitudeUVScale;
  float LongitudeUVOffset;
  float LatitudeUVScale;
  float LatitudeUVOffset;
  
  uint LongitudeRangeFlag;
  bool LongitudeMinHasDiscontinuity;
  bool LongitudeMaxHasDiscontinuity;
  bool LongitudeIsReversed;
  uint LatitudeMinFlag;
  uint LatitudeMaxFlag;

  /**
  * Converts a position from Unit Shape Space coordinates to UV Space.
  * [-1, -1] => [0, 1]
  */
  float3 UnitToUV(float3 UnitPosition)
  {
    return 0.5 * UnitPosition + 0.5;
  }

  /**
  * Converts a position from UV Space coordinates to Unit Shape Space.
  * [0, 1] => [-1, -1]
  */
  float3 UVToUnit(float3 UVPosition)
  {
    return 2.0 * UVPosition - 1.0;
  }

  /**
  * Interpret the input bounds (Local Space) according to the voxel grid shape.
  */
  void Initialize(in int InShapeConstant, in float3 InMinBounds, in float3 InMaxBounds, in float4 PackedData0, in float4 PackedData1, in float4 PackedData2, in float4 PackedData3, in float4 PackedData4, in float4 PackedData5)
  {
    ShapeConstant = InShapeConstant;
    ListState = (IntersectionListState) 0;
    
    if (ShapeConstant == BOX)
    {
      // Default unit box bounds.
      MinBounds = float3(-1, -1, -1);
      MaxBounds = float3(1, 1, 1);
    }
    else if (ShapeConstant == ELLIPSOID)
    {
      MinBounds = InMinBounds; // longitude, sine(latitude), height
      MaxBounds = InMaxBounds; // longitude, sine(latitude), height

      // Flags are packed in CesiumVoxelRendererComponent.cpp
      LongitudeRangeFlag = round(PackedData0.x);
      LongitudeMinHasDiscontinuity = bool(PackedData0.y);
      LongitudeMaxHasDiscontinuity = bool(PackedData0.z);
      LongitudeIsReversed = bool(PackedData0.w);
      
      LatitudeMinFlag = round(PackedData1.x);
      LatitudeMaxFlag = round(PackedData1.y);

      EvoluteScale = PackedData1.zw;
      RadiiUV = PackedData2.xyz;
      InverseRadiiUVSquared = PackedData3.xyz;
      InverseHeightDifferenceUV = PackedData3.w;
      EccentricitySquared = PackedData4.w;
      LongitudeUVExtents = PackedData4.xyz;
      LongitudeUVScale = PackedData5.x;
      LongitudeUVOffset = PackedData5.y;
      LatitudeUVScale = PackedData5.z;
      LatitudeUVOffset = PackedData5.w;
    }
  }
  
  /**
   * Tests whether the input ray (Unit Space) intersects the box. Outputs the intersections in Unit Space.
   */
  void IntersectBox(in Ray R, out float4 Intersections[INTERSECTIONS_LENGTH])
  {
    ListState.Length = 2;
    
    // Consider the box as the intersection of the space between 3 pairs of parallel planes.

    // Compute the distance along the ray to each plane.
    float3 t0 = (MinBounds - R.Origin) / R.Direction;
    float3 t1 = (MaxBounds - R.Origin) / R.Direction;

    // Identify candidate entries/exits based on distance from ray position.
    float3 entries = min(t0, t1);
    float3 exits = max(t0, t1);

    // The actual intersection points are the furthest entry and the closest exit.
    // Do not allow intersections to go behind the shape (negative t).
    RayIntersections result = (RayIntersections) 0;
    float entryT = max(Utils.MaxComponent(entries), 0);
    float exitT = max(Utils.MinComponent(exits), 0);

    if (entryT > exitT)
    {
      result.Entry.t = NO_HIT;
      result.Exit.t = NO_HIT;
      setShapeIntersections(Intersections, ListState, 0, result);
      return;
    }

    // Compute normals
    float3 directions = sign(R.Direction);
    bool3 isLastEntry = bool3(Utils.Equal(entries, float3(entryT, entryT, entryT)));
    result.Entry.Normal = -1.0 * float3(isLastEntry) * directions;
    result.Entry.t = entryT;
   
    bool3 isFirstExit = bool3(Utils.Equal(exits, float3(exitT, exitT, exitT)));
    result.Exit.Normal = float3(isFirstExit) * directions;
    result.Exit.t = exitT;
    
    setShapeIntersections(Intersections, ListState, 0, result);
  }

  /**
  * Tests whether the input ray intersects the ellipsoid at the given height, relative to original radii.
  */
  RayIntersections IntersectEllipsoidAtHeight(in Ray R, in float RelativeHeight, in bool IsConvex)
  {
    // Scale the ray by the ellipsoid axes to make it a unit sphere.
    // Note: This approximates ellipsoid + height as an ellipsoid.
    float3 radiiCorrection = RadiiUV / (RadiiUV + RelativeHeight);
    float3 position = R.Origin * radiiCorrection;
    float3 direction = R.Direction * radiiCorrection;

    // Intersect the ellipsoid defined by x^2/a^2 + y^2/b^2 + z^2/c^2 = 1.
    // Solve for the values of t where the ray intersects the ellipsoid, by substituting
    // the ray equation o + t * d into the ellipsoid x/y values. Results in a quadratic
    // equation on t where we can find the determinant.
    float a = dot(direction, direction); // ~ 1.0 (or maybe 4.0 if ray is scaled)
    float b = dot(direction, position); // roughly inside [-1.0, 1.0] when zoomed in
    float c = dot(position, position) - 1.0; // ~ 0.0 when zoomed in.
    float determinant = b * b - a * c; // ~ b * b when zoomed in

    if (determinant < 0.0)
    {
      // Output a "normal" so that the shape entry information can be encoded.
      // (See EncodeIntersection)
      Intersection miss = Utils.NewIntersection(NO_HIT, normalize(direction));
      return Utils.NewRayIntersections(miss, miss);
    }

    determinant = sqrt(determinant);
    RayIntersections result = (RayIntersections) 0;

    // Compute the larger t using standard formula.
    // The other t may suffer from subtractive cancellation in the standard formula,
    // so it is computed from the first t instead.
    float t1 = (-b - sign(b) * determinant) / a;
    float t2 = c / (a * t1);
    result.Entry.t = min(t1, t2);
    result.Exit.t = max(t1, t2);
  
    float directionScale = IsConvex ? 1.0 : -1.0;
    result.Entry.Normal = directionScale * normalize(position + result.Entry.t * direction);
    result.Exit.Normal = directionScale * normalize(position + result.Exit.t * direction);
    return result;
  }
  
  /**
  * Intersects the plane at the specified longitude angle.
  */
  Intersection IntersectLongitudePlane(in Ray R, in float Angle, in bool PositiveNormal)
  {
    float normalSign = PositiveNormal ? 1.0 : -1.0;
    float2 planeNormal = normalSign * float2(-sin(Angle), cos(Angle));

    float approachRate = dot(R.Direction.xy, planeNormal);
    float distance = -dot(R.Origin.xy, planeNormal);

    float t = (approachRate == 0.0)
        ? NO_HIT
        : distance / approachRate;

    return Utils.NewIntersection(t, float3(planeNormal, 0));
  }
  
  /**
  * Intersects the space on one side of the specified longitude angle.
  */
  RayIntersections IntersectHalfSpace(in Ray R, in float Angle, in bool PositiveNormal)
  {
    Intersection intersection = IntersectLongitudePlane(R, Angle, PositiveNormal);
    Intersection farSide = Utils.NewIntersection(INF_HIT, normalize(R.Direction));

    bool hitFront = (intersection.t > 0.0) == (dot(R.Origin.xy, intersection.Normal.xy) > 0.0);
    if (!hitFront)
    {
      return Utils.NewRayIntersections(intersection, farSide);
    }
    else
    {
      return Utils.NewRayIntersections(Utils.Multiply(farSide, -1), intersection);
    }
  }

  /**
  * Intersects a "flipped" wedge formed by the negative space of the specified angle
  * bounds and returns up to four intersections. The "flipped" wedge is the union of
  * two half-spaces defined at the given angles and represents a *negative* volume
  * of over > 180 degrees.
  */
  void IntersectFlippedWedge(in Ray R, in float2 AngleBounds, out RayIntersections FirstIntersection, out RayIntersections SecondIntersection)
  {
    FirstIntersection = IntersectHalfSpace(R, AngleBounds.x, false);
    SecondIntersection = IntersectHalfSpace(R, AngleBounds.y, true);
  }

  /**
  * Intersects the wedge formed by the negative space of the min/max longitude, where
  * maxAngle > minAngle + pi. The wedge is represented by two planes at such angles.
  * There is an opposite "shadow wedge", i.e. the wedge formed at the *opposite* side
  * of the planes' intersection, that must be specially handled.
  */
  RayIntersections IntersectRegularWedge(in Ray R, in float2 AngleBounds)
  {
    // Normals will point toward the "outside" (into the negative space)
    Intersection intersect1 = IntersectLongitudePlane(R, AngleBounds.x, false);
    Intersection intersect2 = IntersectLongitudePlane(R, AngleBounds.y, true);

    // Note: the intersections could be in the "shadow" wedge, beyond the tip of
    // the actual wedge.
    Intersection first = intersect1;
    Intersection last = intersect2;
    if (first.t > last.t)
    {
      first = intersect2;
      last = intersect1;
    }

    bool firstIntersectionAheadOfRay = first.t >= 0.0;
    bool startedInsideFirst = dot(R.Origin.xy, first.Normal.xy) < 0.0;
    bool isExitingFromInside = firstIntersectionAheadOfRay == startedInsideFirst;
    bool lastIntersectionAheadOfRay = last.t > 0.0;
    bool startedOutsideLast = dot(R.Origin.xy, last.Normal.xy) >= 0.0;
    bool isEnteringFromOutside = lastIntersectionAheadOfRay == startedOutsideLast;

    Intersection farSide = Utils.NewIntersection(INF_HIT, normalize(R.Direction));
    Intersection miss = Utils.NewIntersection(NO_HIT, normalize(R.Direction));

    if (isExitingFromInside && isEnteringFromOutside)
    {
      // Ray crosses both faces of negative wedge, exiting then entering the positive shape
      return Utils.NewRayIntersections(first, last);
    }
    else if (!isExitingFromInside && isEnteringFromOutside)
    {
      // Ray starts inside wedge. last is in shadow wedge, and first is actually the entry
      return Utils.NewRayIntersections(Utils.Multiply(farSide, -1), first);
    }
    else if (isExitingFromInside && !isEnteringFromOutside)
    {
      // First intersection was in the shadow wedge, so last is actually the exit
      return Utils.NewRayIntersections(last, farSide);
    }
    else
    { // !exitFromInside && !enterFromOutside
      // Both intersections were in the shadow wedge
      return Utils.NewRayIntersections(miss, miss);
    }
  }
  
  bool HitsPositiveHalfPlane(in Ray R, in Intersection InputIntersection, in bool positiveNormal)
  {
    float normalSign = positiveNormal ? 1.0 : -1.0;
    float2 planeDirection = float2(InputIntersection.Normal.y, -InputIntersection.Normal.x) * normalSign;
    float2 hit = R.Origin.xy + InputIntersection.t * R.Direction.xy;
    return dot(hit, planeDirection) > 0.0;
  }

  void IntersectHalfPlane(in Ray R, in float Angle, out RayIntersections FirstIntersection, out RayIntersections SecondIntersection)
  {
    Intersection intersection = IntersectLongitudePlane(R, Angle, true);
    Intersection farSide = Utils.NewIntersection(INF_HIT, normalize(R.Direction));

    if (HitsPositiveHalfPlane(R, intersection, true))
    {
      FirstIntersection.Entry = Utils.Multiply(farSide, -1);
      FirstIntersection.Exit = Utils.NewIntersection(intersection.t, float3(-1.0 * intersection.Normal.xy, 0.0));
      SecondIntersection.Entry = intersection;
      SecondIntersection.Exit = farSide;
    }
    else
    {
      Intersection miss = Utils.NewIntersection(NO_HIT, normalize(R.Direction));
      FirstIntersection.Entry = Utils.Multiply(farSide, -1);
      FirstIntersection.Exit = farSide;
      SecondIntersection.Entry = miss;
      SecondIntersection.Exit = miss;
    }
  }

  /**
  * Given a circular quadric cone around the z-axis, with apex at the origin,
  * find the parametric distance(s) along a ray where that ray intersects
  * the cone. Computation of the normal is deferred to GetFlippedCone.
  * 
  * The cone opening angle is described by the squared cosine of its half-angle
  * (the angle between the Z-axis and the surface)
  */
  RayIntersections IntersectQuadricCone(in Ray R, in float cosSquaredHalfAngle)
  {
    float3 o = R.Origin;
    float3 d = R.Direction;
    
    float sinSquaredHalfAngle = 1.0 - cosSquaredHalfAngle;

    float aSin = d.z * d.z * sinSquaredHalfAngle;
    float aCos = -dot(d.xy, d.xy) * cosSquaredHalfAngle;
    float a = aSin + aCos;

    float bSin = d.z * o.z * sinSquaredHalfAngle;
    float bCos = -dot(o.xy, d.xy) * cosSquaredHalfAngle;
    float b = bSin + bCos;

    float cSin = o.z * o.z * sinSquaredHalfAngle;
    float cCos = -dot(o.xy, o.xy) * cosSquaredHalfAngle;
    float c = cSin + cCos;
    // determinant = b * b - a * c. But bSin * bSin = aSin * cSin.
    // Avoid subtractive cancellation by expanding to eliminate these terms
    float determinant = 2.0 * bSin * bCos + bCos * bCos - aSin * cCos - aCos * cSin - aCos * cCos;

    RayIntersections result = (RayIntersections) 0;
    result.Entry.t = NO_HIT;
    result.Exit.t = NO_HIT;

    if (determinant < 0.0)
    {
      return result;
    }

    if (a == 0.0)
    {
      // Ray is parallel to cone surface if b == 0.0.
      // Otherwise, ray has a single intersection on cone surface
      if (b != 0.0)
      {
        result.Entry.t = -0.5 * c / b;
      }
    }

    determinant = sqrt(determinant);

    // Compute larger root using standard formula
    float signB = b < 0.0 ? -1.0 : 1.0;
    float t1 = (-b - signB * determinant) / a;
    // The other root may suffer from subtractive cancellation in the standard formula.
    // Compute it from the first root instead.
    float t2 = c / (a * t1);
    result.Entry.t = min(t1, t2);
    result.Exit.t = max(t1, t2);
    return result;
  }

  /**
   * Given a point on a conical surface, find the surface normal at that point.
   */
  float3 GetConeNormal(in float3 Point, in bool IsConvex)
  {
    // Start with radial component pointing toward z-axis
    float2 radial = -abs(Point.z) * normalize(Point.xy);
    // Z component points toward opening of cone
    float zSign = (Point.z < 0.0) ? -1.0 : 1.0;
    float z = length(Point.xy) * zSign;
    // Flip normal if the shape is convex
    float flip = (IsConvex) ? -1.0 : 1.0;
    return normalize(float3(radial, z) * flip);
  }

  /**
   * Compute the shift between the ellipsoid origin and the apex of a cone of latitude
   */
  float GetLatitudeConeShift(in float sinLatitude)
  {
    // Find prime vertical radius of curvature: 
    // the distance along the ellipsoid normal to the intersection with the z-axis
    float x2 = EccentricitySquared * sinLatitude * sinLatitude;
    float primeVerticalRadius = rsqrt(1.0 - x2);

    // Compute a shift from the origin to the intersection of the cone with the z-axis
    return primeVerticalRadius * EccentricitySquared * sinLatitude;
  }

  /**
  * Tests if the input ray intersects the negative space outside of the cone. In other words,
  * this captures the volume outside the cone, excluding the part of the ray that is inside the cone.
  */ 
  void IntersectFlippedCone(in Ray R, in float CosHalfAngle, out RayIntersections FirstIntersections, out RayIntersections SecondIntersections)
  {
    // Undo the scaling from ellipsoid to sphere
    float3 position = R.Origin * RadiiUV;
    float3 direction = R.Direction * RadiiUV;
    // Shift the ray to account for the latitude cone not being centered at the Earth center
    position.z += GetLatitudeConeShift(CosHalfAngle);

    RayIntersections quadricIntersections = IntersectQuadricCone(R, CosHalfAngle * CosHalfAngle);
    
    Intersection miss = Utils.NewIntersection(NO_HIT, normalize(direction));
    Intersection farSide = Utils.NewIntersection(INF_HIT, normalize(direction));

    // Initialize output with no intersections
    FirstIntersections = (RayIntersections) 0;
    FirstIntersections.Entry = Utils.Multiply(farSide, -1);
    FirstIntersections.Exit = farSide;
    
    SecondIntersections = Utils.NewRayIntersections(miss, miss);
    
    if (quadricIntersections.Entry.t == NO_HIT)
    {
      return;
    }
    
    // Find the points of intersection
    float3 p0 = position + quadricIntersections.Entry.t * direction;
    float3 p1 = position + quadricIntersections.Exit.t * direction;

    quadricIntersections.Entry.Normal = GetConeNormal(p0, true);
    quadricIntersections.Exit.Normal = GetConeNormal(p1, true);

    // shadow cone = the half of the quadric cone that is being discarded.
    bool p0InShadowCone = sign(p0.z) != sign(CosHalfAngle);
    bool p1InShadowCone = sign(p1.z) != sign(CosHalfAngle);

    if (p0InShadowCone && p1InShadowCone)
    {
      // both points in shadow cone; no valid intersections
      return;
    }
    else if (p0InShadowCone)
    {
      FirstIntersections.Exit = quadricIntersections.Exit;
    }
    else if (p1InShadowCone)
    {
      FirstIntersections.Entry = quadricIntersections.Entry;
    }
    else
    {
      FirstIntersections.Exit = quadricIntersections.Entry;
      SecondIntersections.Entry = quadricIntersections.Exit;
      SecondIntersections.Exit = farSide;
    }
  }
  
  /**
  * Tests if the input ray intersects the volume inside the quadric cone. The cone's orientation is
  * dependent on the sign of the input angle.
  */ 
  RayIntersections IntersectCone(in Ray R, in float CosHalfAngle)
  {
    // Undo the scaling from ellipsoid to sphere
    float3 position = R.Origin * RadiiUV;
    float3 direction = R.Direction * RadiiUV;
    // Shift the ray to account for the latitude cone not being centered at the Earth center
    position.z += GetLatitudeConeShift(CosHalfAngle);

    RayIntersections quadricIntersections = IntersectQuadricCone(R, CosHalfAngle * CosHalfAngle);
    
    Intersection miss = Utils.NewIntersection(NO_HIT, normalize(direction));
    Intersection farSide = Utils.NewIntersection(INF_HIT, normalize(direction));
    
    if (quadricIntersections.Entry.t == NO_HIT)
    {
      return Utils.NewRayIntersections(miss, miss);
    }
    
    // Find the points of intersection
    float3 p0 = position + quadricIntersections.Entry.t * direction;
    float3 p1 = position + quadricIntersections.Exit.t * direction;

    quadricIntersections.Entry.Normal = GetConeNormal(p0, false);
    quadricIntersections.Exit.Normal = GetConeNormal(p1, false);

    bool p0InShadowCone = sign(p0.z) != sign(CosHalfAngle);
    bool p1InShadowCone = sign(p1.z) != sign(CosHalfAngle);

    if (p0InShadowCone && p1InShadowCone)
    {
      return Utils.NewRayIntersections(miss, miss);
    }
    else if (p0InShadowCone)
    {
      return Utils.NewRayIntersections(quadricIntersections.Exit, farSide);
    }
    else if (p1InShadowCone)
    {
      return Utils.NewRayIntersections(Utils.Multiply(farSide, -1), quadricIntersections.Entry);
    }
    else
    {
      return Utils.NewRayIntersections(quadricIntersections.Entry, quadricIntersections.Exit);
    }
  }

  /**
  * Intersects an XY-plane at Z = 0. The given Z describes the direction of plane's normal.
  */
  RayIntersections IntersectZPlane(in Ray R, in float Z)
  {
    float t = -R.Origin.z / R.Direction.z;

    bool startsOutside = sign(R.Origin.z) == sign(Z);
    bool isEntering = (t >= 0.0) != startsOutside;

    Intersection intersect = Utils.NewIntersection(t, float3(0.0, 0.0, Z));
    Intersection farSide = Utils.NewIntersection(INF_HIT, normalize(R.Direction));

    if (isEntering)
    {
      return Utils.NewRayIntersections(intersect, farSide);
    }
    else
    {
      return Utils.NewRayIntersections(Utils.Multiply(farSide, -1), intersect);
    }
  }
  
#define ANGLE_EQUAL_ZERO 1
#define ANGLE_UNDER_HALF 2
#define ANGLE_HALF 3
#define ANGLE_OVER_HALF 4

  /**
  * Tests whether the input ray (Unit Space) intersects the ellipsoid region. Outputs the intersections in Unit Space.
  */
  void IntersectEllipsoid(in Ray R, inout float4 Intersections[INTERSECTIONS_LENGTH])
  {
    ListState.Length = 2;

    // The region can be thought of as the space between two ellipsoids.
    RayIntersections OuterResult = IntersectEllipsoidAtHeight(R, MaxBounds.z, true);
    setShapeIntersections(Intersections, ListState, 0, OuterResult);
    if (OuterResult.Entry.t == NO_HIT)
    {
      return;
    }

    ListState.Length += 2;
    RayIntersections InnerResult = IntersectEllipsoidAtHeight(R, MinBounds.z, false);
    if (InnerResult.Entry.t == NO_HIT)
    {
      setShapeIntersections(Intersections, ListState, 1, InnerResult);
    }
    else
    {
      // When the ellipsoid is large and thin it's possible for floating point math
      // to cause the ray to intersect the inner ellipsoid before the outer ellipsoid. 
      // To prevent this from happening, clamp the inner intersections to the outer ones
      // and sandwich the inner ellipsoid intersection inside the outer ellipsoid intersection.
      //
      // Without this special case,
      // [outerMin, outerMax, innerMin, innerMax] will bubble sort to
      // [outerMin, innerMin, outerMax, innerMax] which will cause the back
      // side of the ellipsoid to be invisible because it will think the ray
      // is still inside the inner (negative) ellipsoid after exiting the
      // outer (positive) ellipsoid.
      //
      // With this special case,
      // [outerMin, innerMin, innerMax, outerMax] will bubble sort to
      // [outerMin, innerMin, innerMax, outerMax] which will work correctly.
      //
      // Note: If Sort() changes its sorting function
      // from bubble sort to something else, this code may need to change.
      InnerResult.Entry.t = max(InnerResult.Entry.t, OuterResult.Entry.t);
      InnerResult.Exit.t = min(InnerResult.Exit.t, OuterResult.Exit.t);
      setSurfaceIntersection(Intersections, ListState, 0, OuterResult.Entry, true, true); // positive, entering
      setSurfaceIntersection(Intersections, ListState, 1, InnerResult.Entry, false, true); // negative, entering
      setSurfaceIntersection(Intersections, ListState, 2, InnerResult.Exit, false, false); // negative, exiting
      setSurfaceIntersection(Intersections, ListState, 3, OuterResult.Exit, true, false); // positive, exiting
    }

    // For min latitude, intersect a NEGATIVE cone at the bottom, based on the latitude value.
    if (LatitudeMinFlag == ANGLE_UNDER_HALF)
    {
      ListState.Length += 2;
      RayIntersections BottomConeResult = IntersectCone(R, MinBounds.y);
      setShapeIntersections(Intersections, ListState, 2, BottomConeResult);
    }
    else if (LatitudeMinFlag == ANGLE_HALF)
    {
      ListState.Length += 2;
      RayIntersections PlaneResult = IntersectZPlane(R, -1.0);
      setShapeIntersections(Intersections, ListState, 2, PlaneResult);
    }
    else if (LatitudeMinFlag == ANGLE_OVER_HALF)
    {
      ListState.Length += 4;
      RayIntersections FirstIntersection;
      RayIntersections SecondIntersection;
      IntersectFlippedCone(R, MinBounds.y, FirstIntersection, SecondIntersection);
      setShapeIntersections(Intersections, ListState, 2, FirstIntersection);
      setShapeIntersections(Intersections, ListState, 3, SecondIntersection);
    }

    // For max latitude, intersect a NEGATIVE cone at the top, based on the latitude value.
    if (LatitudeMaxFlag == ANGLE_UNDER_HALF)
    {
      RayIntersections FirstIntersection;
      RayIntersections SecondIntersection;
      IntersectFlippedCone(R, MaxBounds.y, FirstIntersection, SecondIntersection);
      // The array index depends on how many intersections were previously found.
      [branch]
      switch (ListState.Length)
      {
        case 6: // maxHeight + minHeight + minLatitude <= half
          setShapeIntersections(Intersections, ListState, 3, FirstIntersection);
          setShapeIntersections(Intersections, ListState, 4, SecondIntersection);
          break;
        case 8: // maxHeight + minHeight + minLatitude > half
          // It makes no sense for min latitude to be in the top half, AND max latitude in the bottom half.
          // The region is invalid, so make things easier and mark the shape as not hit.
          Intersections[0].w = NO_HIT;
          Intersections[1].w = NO_HIT;
          ListState.Length = 0;
          return;
        case 4: // maxHeight + minHeight
        default:
          setShapeIntersections(Intersections, ListState, 2, FirstIntersection);
          setShapeIntersections(Intersections, ListState, 3, SecondIntersection);
          break;
      }
      ListState.Length += 4;
    }
    else if (LatitudeMaxFlag == ANGLE_HALF)
    {
      RayIntersections PlaneResult = IntersectZPlane(R, 1.0);
      // The array index depends on how many intersections were previously found.
      [branch]
      switch (ListState.Length)
      {
        case 6: // maxHeight + minHeight + minLatitude <= half
          // Not worth checking if minLatitude == half, just let the sort algorithm figure it out
          setShapeIntersections(Intersections, ListState, 3, PlaneResult);
          break;
        case 8: // maxHeight + minHeight + minLatitude > half
          // It makes no sense for min latitude to be over half, AND max latitude to be half.
          // The region is invalid, so make things easier and mark the shape as not hit.
          Intersections[0].w = NO_HIT;
          Intersections[1].w = NO_HIT;
          ListState.Length = 0;
          return;
        case 4: // maxHeight + minHeight
        default:
          setShapeIntersections(Intersections, ListState, 2, PlaneResult);
          break;
      }
      ListState.Length += 2;
    }
    else if (LatitudeMaxFlag == ANGLE_OVER_HALF)
    {
      RayIntersections TopConeResult = IntersectCone(R, MaxBounds.y);
      [branch]
      switch (ListState.Length)
      {
        case 6: // maxHeight + minHeight + minLatitude <= half
          setShapeIntersections(Intersections, ListState, 3, TopConeResult);
          break;
        case 8: // maxHeight + minHeight + minLatitude > half
          setShapeIntersections(Intersections, ListState, 4, TopConeResult);
          break;
        case 4: // maxHeight + minHeight
        default:
          setShapeIntersections(Intersections, ListState, 2, TopConeResult);
          break;
      }
      ListState.Length += 2;
    }
    
    float2 LongitudeBounds = float2(MinBounds.x, MaxBounds.x);
    if (LongitudeRangeFlag == ANGLE_OVER_HALF)
    {
      // The shape's longitude range is over half, so we intersect a NEGATIVE shape that is under half.
      RayIntersections WedgeResult = IntersectRegularWedge(R, LongitudeBounds);
      // The array index depends on how many intersections were previously found.
      [branch]
      switch (ListState.Length)
      {
        case 6: // maxHeight + minHeight + (minLatitude <= half || maxLatitude >= half)
          setShapeIntersections(Intersections, ListState, 3, WedgeResult);
          break;
        case 8: // maxHeight + minHeight + (minLatitude > half || maxLatitude < half || minLatitude <= half && maxLatitude >= half)
          setShapeIntersections(Intersections, ListState, 4, WedgeResult);
          break;
        case 10: // maxHeight + minHeight + (minLatitude > half && maxLatitude > half || minLatitude < half + maxLatitude < half)
          setShapeIntersections(Intersections, ListState, 5, WedgeResult);
          break;
        case 4: // maxHeight + minHeight
        default:
          setShapeIntersections(Intersections, ListState, 2, WedgeResult);
          break;
      }
      ListState.Length += 2;
    }
    else if (LongitudeRangeFlag == ANGLE_UNDER_HALF)
    {
      // The shape's longitude range is under half, so we intersect a NEGATIVE shape that is over half.
      RayIntersections FirstResult = (RayIntersections) 0;
      RayIntersections SecondResult = (RayIntersections) 0;
      IntersectFlippedWedge(R, LongitudeBounds, FirstResult, SecondResult);
      // The array index depends on how many intersections were previously found.
      [branch]
      switch (ListState.Length)
      {
        case 6: // maxHeight + minHeight + (minLatitude <= half || maxLatitude >= half)
          setShapeIntersections(Intersections, ListState, 3, FirstResult);
          setShapeIntersections(Intersections, ListState, 4, SecondResult);
          break;
        case 8: // maxHeight + minHeight + (minLatitude > half || maxLatitude < half || minLatitude <= half && maxLatitude >= half)
          setShapeIntersections(Intersections, ListState, 4, FirstResult);
          setShapeIntersections(Intersections, ListState, 5, SecondResult);
          break;
        case 10: // maxHeight + minHeight + (minLatitude > half && maxLatitude > half || minLatitude < half + maxLatitude < half)
          setShapeIntersections(Intersections, ListState, 5, FirstResult);
          setShapeIntersections(Intersections, ListState, 6, SecondResult);
          break;
        case 4: // maxHeight + minHeight
        default:
          setShapeIntersections(Intersections, ListState, 2, FirstResult);
          setShapeIntersections(Intersections, ListState, 3, SecondResult);
          break;
      }
      ListState.Length += 4;
    }
    else if (LongitudeRangeFlag == ANGLE_EQUAL_ZERO)
    {
      RayIntersections FirstResult = (RayIntersections) 0;
      RayIntersections SecondResult = (RayIntersections) 0;
      IntersectHalfPlane(R, LongitudeBounds.x, FirstResult, SecondResult);
      // The array index depends on how many intersections were previously found.
      [branch]
      switch (ListState.Length)
      {
        case 6: // maxHeight + minHeight + (minLatitude <= half || maxLatitude >= half)
          setShapeIntersections(Intersections, ListState, 3, FirstResult);
          setShapeIntersections(Intersections, ListState, 4, SecondResult);
          break;
        case 8: // maxHeight + minHeight + (minLatitude > half || maxLatitude < half || minLatitude <= half && maxLatitude >= half)
          setShapeIntersections(Intersections, ListState, 4, FirstResult);
          setShapeIntersections(Intersections, ListState, 5, SecondResult);
          break;
        case 10: // maxHeight + minHeight + (minLatitude > half && maxLatitude > half || minLatitude < half + maxLatitude < half)
          setShapeIntersections(Intersections, ListState, 5, FirstResult);
          setShapeIntersections(Intersections, ListState, 6, SecondResult);
          break;
        case 4: // maxHeight + minHeight
        default:
          setShapeIntersections(Intersections, ListState, 2, FirstResult);
          setShapeIntersections(Intersections, ListState, 3, SecondResult);
          break;
      }
      ListState.Length += 4;
    }
  }
  
  /**
  * Tests whether the input ray (Unit Space) intersects the shape.
  */
  RayIntersections
    IntersectShape(in Ray R, out float4 Intersections[INTERSECTIONS_LENGTH])
  {
    [branch]
    switch (ShapeConstant)
    {
      case BOX:
        IntersectBox(R, Intersections);
        break;
      case ELLIPSOID:
        IntersectEllipsoid(R, Intersections);
        break;
      default:
        return Utils.NewRayIntersections(Utils.NewIntersection(NO_HIT, 0), Utils.NewIntersection(NO_HIT, 0));
    }
    
    RayIntersections result = ListState.GetFirstIntersections(Intersections);
    if (result.Entry.t == NO_HIT)
    {
      return result;
    }
    
    // Don't bother with sorting if the positive shape was missed.
    // Box intersection is straightforward and doesn't require sorting.
    // Currently, cylinders do not require sorting, but they will once clipping / exaggeration
    // is supported.
    if (ShapeConstant == ELLIPSOID)
    {
      ListState.Sort(Intersections);
      for (int i = 0; i < SHAPE_INTERSECTIONS; ++i)
      {
        result = ListState.GetNextIntersections(Intersections);
        if (result.Exit.t > 0.0)
        {
          // Set start to 0.0 when ray is inside the shape.
          result.Entry.t = max(result.Entry.t, 0.0);
          break;
        }
      }
    }
    else
    {
      // Set start to 0.0 when ray is inside the shape.
      result.Entry.t = max(result.Entry.t, 0.0);
    }

    return result;
  }
  
  /**
  * Scales the input UV coordinates from [0, 1] to their values in UV Shape Space.  
  */
  float3 ScaleUVToShapeUVSpace(in float3 UV)
  {
    if (ShapeConstant == ELLIPSOID)
    {
      // Convert from [0, 1] to radians [-pi, pi]
      float longitude = UV.x * CZM_TWO_PI;
      if (LongitudeRangeFlag > 0)
      {
        longitude /= LongitudeUVScale;
      }

      // Convert from [0, 1] to radians [-pi/2, pi/2]
      float latitude = UV.y * CZM_PI;
      if (LatitudeMinFlag > 0 || LatitudeMaxFlag > 0)
      {
        latitude /= LatitudeUVScale;
      }
    
      float height = UV.z / InverseHeightDifferenceUV;

      return float3(longitude, latitude, height);
    }
    else
    {
      // For CYLINDER: Once clipping / exaggeration is supported, an offset + scale
      // should be applied to its radius / height / angle. (See CesiumJS)
      return UV;
    }
  }

  /**
   * Converts the input position (vanilla UV Space) to its Shape UV Space relative to the
   * box geometry. Also outputs the Jacobian transpose for future use.
   */
  float3 ConvertUVToShapeUVSpaceBox(in float3 PositionUV, out float3x3 JacobianT)
  {
    // For Box, Cartesian UV space = UV shape space, so we can use PositionUV as-is.
    // The Jacobian is the identity matrix, except that a step of 1 only spans half the shape
    // space [-1, 1], so the identity is scaled.
    JacobianT = float3x3(0.5f, 0, 0, 0, 0.5f, 0, 0, 0, 0.5f);
    return PositionUV;
  }

  /**
    * Given a specified point, gets the nearest point (XY) on the ellipse using a robust
    * iterative solution without trig functions, as well as the radius of curvature
    * at that point (Z).
    * https://github.com/0xfaded/ellipse_demo/issues/1
    * https://stackoverflow.com/questions/22959698/distance-from-given-point-to-given-ellipse
    */
  float3 GetNearestPointAndRadiusOnEllipse(float2 pos, float2 radii)
  {
    float2 p = abs(pos);
    float2 inverseRadii = 1.0 / radii;

    // We describe the ellipse parametrically: v = radii * vec2(cos(t), sin(t))
    // but store the cos and sin of t in a vec2 for efficiency.
    // Initial guess: t = pi/4
    float2 tTrigs = 0.7071067811865476;
    // Initial guess of point on ellipsoid
    float2 v = radii * tTrigs;
    // Center of curvature of the ellipse at v
    float2 evolute = EvoluteScale * tTrigs * tTrigs * tTrigs;

    const int iterations = 3;
    for (int i = 0; i < iterations; ++i)
    {
      // Find the (approximate) intersection of p - evolute with the ellipsoid.
      float2 q = normalize(p - evolute) * length(v - evolute);
      // Update the estimate of t.
      tTrigs = (q + evolute) * inverseRadii;
      tTrigs = normalize(clamp(tTrigs, 0.0, 1.0));
      v = radii * tTrigs;
      evolute = EvoluteScale * tTrigs * tTrigs * tTrigs;
    }

    return float3(v * sign(pos), length(v - evolute));
  }
  
  /**
   * Converts the input position (vanilla UV Space) to its Shape UV Space relative to the
   * ellipsoid geometry. Also outputs the Jacobian transpose for future use.
   */
  float3 ConvertUVToShapeUVSpaceEllipsoid(in float3 PositionUV, out float3x3 JacobianT)
  {
    // First convert UV to "Unit Space derivative".
    // Get the position in unit space, undo the scaling from ellipsoid to sphere
    float3 position = UVToUnit(PositionUV);
    position *= RadiiUV;

    float longitude = atan2(position.y, position.x);
    float3 east = normalize(float3(-position.y, position.x, 0.0));

    // Convert the 3D position to a 2D position relative to the ellipse (radii.x, radii.z)
    // (assume radii.y == radii.x) and find the nearest point on the ellipse and its normal
    float distanceFromZAxis = length(position.xy);
    float2 positionEllipse = float2(distanceFromZAxis, position.z);
    float3 surfacePointAndRadius = GetNearestPointAndRadiusOnEllipse(positionEllipse, RadiiUV.xz);
    float2 surfacePoint = surfacePointAndRadius.xy;

    float2 normal2d = normalize(surfacePoint * InverseRadiiUVSquared.xz);
    float latitude = atan2(normal2d.y, normal2d.x);
    float3 north = float3(-normal2d.y * normalize(position.xy), abs(normal2d.x));

    float heightSign = length(positionEllipse) < length(surfacePoint) ? -1.0 : 1.0;
    float height = heightSign * length(positionEllipse - surfacePoint);
    float3 up = normalize(cross(east, north));

    JacobianT = float3x3(east / distanceFromZAxis, north / (surfacePointAndRadius.z + height), up);
    // Transpose because HLSL matrices are constructed in row-major order.
    JacobianT = transpose(JacobianT);

    // Then convert Unit Space to Shape UV Space
    // Longitude: shift & scale to [0, 1]
    float longitudeUV = (longitude + CZM_PI) / CZM_TWO_PI;

    // Correct the angle when max < min
    // Technically this should compare against min longitude - but it has precision problems so compare against the middle of empty space.
    if (LongitudeIsReversed)
    {
      longitudeUV += float(longitudeUV < LongitudeUVExtents.z);
    }

    // Avoid flickering from reading voxels from both sides of the -pi/+pi discontinuity.
    if (LongitudeMinHasDiscontinuity)
    {
      longitudeUV = longitudeUV > LongitudeUVExtents.z ? LongitudeUVExtents.x : longitudeUV;
    }

    if (LongitudeMaxHasDiscontinuity)
    {
      longitudeUV = longitudeUV < LongitudeUVExtents.z ? LongitudeUVExtents.y : longitudeUV;
    }

    if (LongitudeRangeFlag > 0)
    {
      longitudeUV = longitudeUV * LongitudeUVScale + LongitudeUVOffset;
    }

    // Latitude: shift and scale to [0, 1]
    float latitudeUV = (latitude + CZM_PI_OVER_TWO) / CZM_PI;
    if (LatitudeMinFlag > 0 || LatitudeMaxFlag > 0)
    {
      latitudeUV = latitudeUV * LatitudeUVScale + LatitudeUVOffset;
    }

    // Height: scale to the range [0, 1]
    float heightUV = 1.0 + height * InverseHeightDifferenceUV;

    return float3(longitudeUV, latitudeUV, heightUV);
  }
  
  /**
  * Converts the input position (vanilla UV Space) to its Shape UV Space relative to the
  * voxel grid geometry. Also outputs the Jacobian transpose for future use.
  */
  float3 ConvertUVToShapeUVSpace(in float3 UVPosition, out float3x3 JacobianT)
  {
    switch (ShapeConstant)
    {
      case BOX:
        return ConvertUVToShapeUVSpaceBox(UVPosition, JacobianT);
      case ELLIPSOID:
        return ConvertUVToShapeUVSpaceEllipsoid(UVPosition, JacobianT);
      default:
        // Default return
        JacobianT = float3x3(1, 0, 0, 0, 1, 0, 0, 0, 1);
        return UVPosition;
    }
  }
};

/*===========================
  END SHAPE UTILITY
=============================*/

/*===========================
  BEGIN OCTREE TRAVERSAL UTILITY
=============================*/

// An octree node is encoded by 9 texels.
// The first texel contains the index of the node's parent.
// The other texels contain the indices of the node's children.
#define TEXELS_PER_NODE 9

#define OCTREE_MAX_LEVELS 32 // Hardcoded because HLSL doesn't like variable length loops

#define OCTREE_FLAG_EMPTY 0
#define OCTREE_FLAG_LEAF 1
#define OCTREE_FLAG_INTERNAL 2

#define MINIMUM_STEP_SCALAR (0.02)
#define SHIFT_EPSILON (0.001)

struct Node
{
  int Flag;
  // "Data" is interpreted differently depending on the flag of the node.
  // For internal nodes, this refers to the index of the node in the octree texture,
  // where its children data are stored.
  // For leaf nodes, this represents the index in the megatexture where the node's
  // actual metadata is stored.
  int Data;
  // Only applicable to leaf nodes.
  int LevelDifference;
};

/*
 * Representation of an in-progress traversal of an octree. Points to a node at the given coords,
 * and keeps track of the parent in case the traversal needs to be reversed.
 *
 * NOTE the difference between "coordinates" and "index".
 * Octree coordinates are given as int4, representing the X, Y, Z, of the node in its tree level (W).
 * These are not to be confused with the index of a node within the actual texture.
 */
struct OctreeTraversal
{
  int4 Coords; // W = Octree Level (starting from 0); XYZ = Index of node within that level
  int Index;
};

struct TileSample
{
  int Index; // Index of the sample in the voxel data texture.
  int4 Coords;
  float3 LocalUV;
};

struct Octree
{
  Texture2D NodeData;
  uint TextureWidth;
  uint TextureHeight;
  uint TilesPerRow;
  uint3 GridDimensions;

  ShapeUtility ShapeUtils;
  
  /**
   * Sets the octree texture and computes related variables.
   */
  void SetNodeData(in Texture2D DataTexture)
  {
    NodeData = DataTexture;
    NodeData.GetDimensions(TextureWidth, TextureHeight);
    TilesPerRow = TextureWidth / TEXELS_PER_NODE;
  }

  /**
   * Given an octree-relative UV position, converts it to the corresponding position
   * within the UV-space of the *tile* at the specified coordinates.
   */
  float3 GetTileUV(in float3 PositionUV, in int4 Coords)
  {
    // Get the size of the tile at the given level.
    float tileSize = float(1u << Coords.w);
    return PositionUV * tileSize - float3(Coords.xyz);
  }
  
  /**
   * Given an octree-relative UV position, checks whether it is inside the tile
   * specified by the octree coords. Assumes the position is always inside the
   * root tile of the tileset.
   */
  bool IsInsideTile(in float3 PositionUV, in int4 OctreeCoords)
  {
    float3 tileUV = GetTileUV(PositionUV, OctreeCoords);
    bool isInside = ShapeUtils.Utils.IsInRange(tileUV, 0, 1);
    return isInside || OctreeCoords.w == 0;
  }
  
  /**
  * Gets the size of a voxel cell within the given octree level.
  */
  float3 GetVoxelSizeAtLevel(in int Level)
  {
    float3 sampleCount = float(1u << Level) * float3(GridDimensions);
    float3 voxelSizeUV = 1.0 / sampleCount;
    return ShapeUtils.ScaleUVToShapeUVSpace(voxelSizeUV);
  }
  
  TileSample GetSampleFromNode(in Node Node, in float4 OctreeCoords)
  {
    TileSample result = (TileSample) 0;
    result.Index = (Node.Flag != OCTREE_FLAG_EMPTY)
        ? Node.Data
        : -1;
    float denominator = float(1u << Node.LevelDifference);
    result.Coords = int4(OctreeCoords.xyz / denominator, OctreeCoords.w - Node.LevelDifference);
    return result;
  }
  
  Node GetNodeFromTexture(in int2 TextureIndex)
  {
    float4 nodeData = NodeData.Load(int3(TextureIndex.x, TextureIndex.y, 0));
    Node node = (Node) 0;
    node.Flag = int(nodeData.x * 255.0);
    node.LevelDifference = int(nodeData.y * 255.0);
    node.Data = ShapeUtils.Utils.ConstructInt(nodeData.zw);
    return node;
  }
  
  /**
  * Given the index of a node, converts it to texture indices, which are needed to actually
  * retrieve the node data from the octree texture.
  */
  int2 GetTextureIndexOfNode(in int NodeIndex)
  {
    int2 result;
    result.x = (NodeIndex % TilesPerRow) * TEXELS_PER_NODE;
    result.y = NodeIndex / TilesPerRow;
    return result;
  }

  Node GetChildNode(in int NodeIndex, in int3 ChildCoord)
  {
    int childIndex = ChildCoord.z * 4 + ChildCoord.y * 2 + ChildCoord.x;
    int2 textureIndex = GetTextureIndexOfNode(NodeIndex);
    textureIndex.x += 1 + childIndex;
    return GetNodeFromTexture(textureIndex);
  }

  Node GetParentNode(in int NodeIndex)
  {
    return GetNodeFromTexture(GetTextureIndexOfNode(NodeIndex));
  }
  
  /**
  * Given a UV position [0, 1] within the voxel grid, traverse to the leaf containing it.
  * Outputs the node's corresponding data texture index and octree coordinates.
  */
  Node TraverseToLeaf(in float3 PositionUV, inout OctreeTraversal Traversal)
  {
    // Get the size of the node at the current level
    float sizeAtLevel = exp2(-1.0 * float(Traversal.Coords.w));
    float3 start = float3(Traversal.Coords.xyz) * sizeAtLevel;
    float3 end = start + sizeAtLevel;
    
    Node child;

    for (int i = 0; i < OCTREE_MAX_LEVELS; ++i)
    {
      // Find the octree child that contains the given position.
      // Example: the point (0.75, 0.25, 0.75) belongs to the child at coords (1, 0, 1)
      float3 center = 0.5 * (start + end);
      int3 childCoord = step(center, PositionUV);

      // Get the octree coordinates for the next level down.
      Traversal.Coords = int4(Traversal.Coords.xyz * 2 + childCoord, Traversal.Coords.w + 1);

      child = GetChildNode(Traversal.Index, childCoord);

      if (child.Flag != OCTREE_FLAG_INTERNAL)
      {
        // Found leaf - stop traversing
        break;
      }

      // Keep going!
      start = lerp(start, center, childCoord);
      end = lerp(center, end, childCoord);
      Traversal.Index = child.Data;
    }

    return child;
  }

  void BeginTraversal(in float3 PositionUV, out OctreeTraversal Traversal, out TileSample Sample)
  {
    Traversal = (OctreeTraversal) 0;

    // Start from root
    Node currentNode = GetNodeFromTexture(int2(0, 0));
    if (currentNode.Flag == OCTREE_FLAG_INTERNAL)
    {
      currentNode = TraverseToLeaf(PositionUV, Traversal);
    }

    Sample = GetSampleFromNode(currentNode, Traversal.Coords);
    Sample.LocalUV = clamp(GetTileUV(PositionUV, Sample.Coords), 0.0, 1.0);
  }

  void ResumeTraversal(in float3 PositionUV, inout OctreeTraversal Traversal, inout TileSample Sample)
  {
    if (IsInsideTile(PositionUV, Traversal.Coords))
    {
      // Continue to sample the same tile, marching to a different voxel.
      Sample.LocalUV = clamp(GetTileUV(PositionUV, Sample.Coords), 0.0, 1.0);
      return;
    }

    // Otherwise, go up tree until we find a parent tile containing the position.
    for (int i = 0; i < OCTREE_MAX_LEVELS; ++i)
    {
      Traversal.Coords.w -= 1; // Up one level
      Traversal.Coords.xyz /= 2; // Get coordinates of the parent tile.
      
      if (IsInsideTile(PositionUV, Traversal.Coords))
      {
        break;
      }
      Node parent = GetParentNode(Traversal.Index);
      Traversal.Index = parent.Data;
    }

    // Go down tree
    Node node = TraverseToLeaf(PositionUV, Traversal);
    Sample = GetSampleFromNode(node, Traversal.Coords);
    Sample.LocalUV = clamp(GetTileUV(PositionUV, Sample.Coords), 0.0, 1.0);
  }

  /**
  * Given UV coordinates within a tile, and the voxel dimensions along a ray
  * passing through the coordinates, find the intersections where the ray enters
  * and exits the voxel cell.
  *
  * Outputs the distance to the points and the surface normals in UV Shape Space.
  */
  RayIntersections GetVoxelIntersections(in float3 TileUV, in float3 VoxelSizeAlongRay)
  {
    float3 voxelCoord = TileUV * float3(GridDimensions);
    float3 directions = sign(VoxelSizeAlongRay);
    float3 positiveDirections = max(directions, 0.0);
    float3 entryCoord = lerp(ceil(voxelCoord), floor(voxelCoord), positiveDirections);
    float3 exitCoord = entryCoord + directions;

    RayIntersections Intersections;

    float3 distanceFromEntry = -abs((entryCoord - voxelCoord) * VoxelSizeAlongRay);
    float lastEntry = ShapeUtils.Utils.MaxComponent(distanceFromEntry);
    bool3 isLastEntry = ShapeUtils.Utils.Equal(distanceFromEntry, float3(lastEntry, lastEntry, lastEntry));
    Intersections.Entry.Normal = -1.0 * float3(isLastEntry) * directions;
    Intersections.Entry.t = lastEntry;

    float3 distanceToExit = abs((exitCoord - voxelCoord) * VoxelSizeAlongRay);
    float firstExit = ShapeUtils.Utils.MinComponent(distanceToExit);
    bool3 isFirstExit = ShapeUtils.Utils.Equal(distanceToExit, float3(firstExit, firstExit, firstExit));
    Intersections.Exit.Normal = float3(isFirstExit) * directions;
    Intersections.Exit.t = firstExit;

    return Intersections;
  }
  
  /**
   * Computes the next intersection in the octree by stepping to the next voxel cell
   * within the shape along the ray.
   *
   * Outputs the distance relative to the initial intersection, as opposed to the distance from the
   * origin of the ray.
   */
  Intersection GetNextVoxelIntersection(in TileSample Sample, in float3 Direction, in RayIntersections ShapeIntersections, in float3x3 JacobianT, in float CurrentT)
  {
    // The Jacobian is computed in a space where the shape spans [-1, 1].
    // But the ray is marched in a space where the shape fills [0, 1].
    // So we need to scale the Jacobian by 2.
    float3 gradient = 2.0 * mul(Direction, JacobianT);
    float3 voxelSizeAlongRay = GetVoxelSizeAtLevel(Sample.Coords.w) / gradient;
    RayIntersections voxelIntersections = GetVoxelIntersections(Sample.LocalUV, voxelSizeAlongRay);

    // Transform normal from UV Shape Space to Cartesian space. 
    float3 voxelNormal = normalize(mul(JacobianT, voxelIntersections.Entry.Normal));
    // Then, compare with the original shape intersection to choose the appropriate normal.
    Intersection voxelEntry = ShapeUtils.Utils.NewIntersection(CurrentT + voxelIntersections.Entry.t, voxelNormal);
    Intersection entry = ShapeUtils.Utils.Max(ShapeIntersections.Entry, voxelEntry);

    float fixedStep = ShapeUtils.Utils.MinComponent(abs(voxelSizeAlongRay));
    float shift = fixedStep * SHIFT_EPSILON;
    float dt = voxelIntersections.Exit.t + shift;
    if ((CurrentT + dt) > ShapeIntersections.Exit.t)
    {
      // Stop at end of shape.
      dt = ShapeIntersections.Exit.t - CurrentT + shift;
    }
    float stepSize = clamp(dt, fixedStep * MINIMUM_STEP_SCALAR, fixedStep + shift);

    return ShapeUtils.Utils.NewIntersection(stepSize, entry.Normal);
  }
};

/*===========================
  END OCTREE TRAVERSAL UTILITY
=============================*/

/*===========================
  BEGIN VOXEL DATA TEXTURE UTILITY
=============================*/

struct VoxelDataTextures
{
  %s
  int ShapeConstant;
  uint3 TileCount; // Number of tiles in the texture, in three dimensions.

  // NOTE: Unlike Octree, these dimensions are specified with respect to the voxel attributes
  // in the glTF model. For box and cylinder voxels, the YZ dimensions will be swapped.
  uint3 GridDimensions;
  uint3 PaddingBefore;
  uint3 PaddingAfter;

  int3 TileIndexToCoords(in int Index)
  {
    if (TileCount.x == 0 || TileCount.y == 0 || TileCount.z == 0)
    {
      return 0;
    }
    int ZSlice = TileCount.x * TileCount.y;
    int Z = Index / ZSlice;
    int Y = (Index % ZSlice) / TileCount.x;
    int X = Index % TileCount.x;
    return int3(X, Y, Z) * (GridDimensions + PaddingBefore + PaddingAfter);
  }
  
  CustomShaderProperties GetProperties(in TileSample Sample)
  {
    // Compute the tile location within the texture.
    float3 TileCoords = TileIndexToCoords(Sample.Index);
    
    // Compute int coordinates of the voxel within the tile.
    float3 LocalUV = Sample.LocalUV;
    uint3 DataDimensions = GridDimensions + PaddingBefore + PaddingAfter;

    if (ShapeConstant == BOX)
    {
      // Since glTFs are y-up (and 3D Tiles is z-up), the data must be accessed to reflect the transforms
      // from a Y-up to Z-up frame of reference, plus the Cesium -> Unreal transform as well.
      LocalUV = float3(LocalUV.x, clamp(1.0 - LocalUV.z, 0.0, 1.0), LocalUV.y);
    }

    float3 VoxelCoords = floor(LocalUV * float3(GridDimensions));
    // Account for padding
    VoxelCoords = clamp(VoxelCoords + float3(PaddingBefore), 0, float3(DataDimensions - 1u));
    
    int3 Coords = TileCoords + VoxelCoords;
    
    CustomShaderProperties Properties = (CustomShaderProperties) 0;
 %s

    return Properties;
  }
};

/*===========================
  END VOXEL DATA TEXTURE UTILITY
=============================*/

/*===========================
  MAIN FUNCTION BODY
=============================*/

#define STEP_COUNT_MAX 1000
#define ALPHA_ACCUMULATION_MAX 0.98 // Must be > 0.0 and <= 1.0

Octree VoxelOctree;
VoxelOctree.ShapeUtils = (ShapeUtility) 0;
VoxelOctree.ShapeUtils.Initialize(ShapeConstant, ShapeMinBounds, ShapeMaxBounds, PackedData0, PackedData1, PackedData2, PackedData3, PackedData4, PackedData5);

Ray R = (Ray) 0;
R.Origin = RayOrigin;
R.Direction = RayDirection;

// Input ray is Unit Space.
//
// Ultimately, we want to traverse the voxel grid in a [0, 1] UV space to simplify much of
// the voxel octree math. This is simply referred to UV space, or "vanilla" UV space.
// However, this UV space won't always be a perfect voxel cube. It must conform to the specified
// shape of the voxel volume. Voxel cells may be curved (e.g., around a cylinder), or non-uniformly
// scaled. This shape-relative UV space is aptly referred to as Shape UV Space.
//
// Shape UV Space is DIFFERENT from the shape's Unit Space. Think of Unit Space as the perfect
// version of the grid geometry: a solid box, cylinder, or ellipsoid, within [-1, 1].
// The actual voxel volume can be a subsection of that perfect unit space, e.g., a hollowed
// cylinder with radius [0.5, 1].
//
// Shape UV Space is the [0, 1] grid mapping that conforms to the actual voxel volume. Imagine 
// the voxel grid curving concentrically around a unit cylinder, then being "smooshed" to fit in
// the volume of the hollow cylinder. Shape UV Space must account for the angle bounds of a cylinder,
// and the longitude / latitude / height bounds of an ellipsoid.
//
// Therefore, we must convert the unit space ray to the equivalent Shape UV Space ray to sample correctly.
// Spaces will be referred to as follows:
// 
// Unit Space: Unit space of the grid geometry from [-1, 1]. A perfectly solid box, cylinder, or ellipsoid.
// Shape UV Space: Voxel space from [0, 1] conforming to the actual voxel volume. The volume could be a box, a part of a cylinder, or a region on an ellipsoid.
// (Vanilla) UV Space: Voxel space within an untransformed voxel octree. This can be envisioned as a simple cube spanning [0, 1] in three dimensions.

RayIntersections Intersections = VoxelOctree.ShapeUtils.IntersectShape(R, IntersectionList);
if (Intersections.Entry.t == NO_HIT) {
  return 0;
}

// Intersections are returned in Unit Space. Transform to UV space [0, 1] for raymarching through octree.
R.Origin = VoxelOctree.ShapeUtils.UnitToUV(R.Origin);
R.Direction = R.Direction * 0.5;

// Initialize octree
VoxelOctree.SetNodeData(OctreeData);
VoxelOctree.GridDimensions = GridDimensions;
 
// Initialize data textures
VoxelDataTextures DataTextures;
DataTextures.ShapeConstant = ShapeConstant;
DataTextures.TileCount = TileCount;

switch (ShapeConstant) {
  case BOX:
    DataTextures.GridDimensions = round(GridDimensions.xzy);
    DataTextures.PaddingBefore = round(PaddingBefore.xzy);
    DataTextures.PaddingAfter = round(PaddingAfter.xzy);
    break;
  case ELLIPSOID:
  default:
    DataTextures.GridDimensions = round(GridDimensions);
    DataTextures.PaddingBefore = round(PaddingBefore);
    DataTextures.PaddingAfter = round(PaddingAfter);
    break;
}

%s

float CurrentT = Intersections.Entry.t;
float EndT = Intersections.Exit.t;
float3 PositionUV = R.Origin + CurrentT * R.Direction;

// The Jacobian is necessary to compute voxel intersections with respect to the grid's shape.
float3x3 JacobianT;
float3 PositionShapeUVSpace = VoxelOctree.ShapeUtils.ConvertUVToShapeUVSpace(PositionUV, JacobianT);

// For ellipsoids, the UV direction has been scaled to a space where the ellipsoid is a sphere.
// Undo this scaling to get the raw direction.
float3 RawDirection = R.Direction;
if (VoxelOctree.ShapeUtils.ShapeConstant == ELLIPSOID)
{
  RawDirection *= VoxelOctree.ShapeUtils.RadiiUV;
}

OctreeTraversal Traversal;
TileSample Sample;
VoxelOctree.BeginTraversal(PositionShapeUVSpace, Traversal, Sample);
Intersection NextIntersection = VoxelOctree.GetNextVoxelIntersection(Sample, RawDirection, Intersections, JacobianT, CurrentT);

float4 AccumulatedResult = float4(0, 0, 0, 0);

CustomShaderProperties Properties;
CustomShader CS;

for (int step = 0; step < STEP_COUNT_MAX; step++) {
  if (Sample.Index >= 0) {
    Properties = DataTextures.GetProperties(Sample);
    // TODO: expose additional properties?
    float4 result = CS.Shade(Properties);
    AccumulatedResult += result;

    // Stop traversing if the alpha has been fully saturated.
    if (AccumulatedResult.w > ALPHA_ACCUMULATION_MAX) {
      AccumulatedResult.w = ALPHA_ACCUMULATION_MAX;
      break;
    }
  }

  if (NextIntersection.t <= 0.0) {
    // Shape is infinitely thin. The ray may have hit the edge of a
    // foreground voxel. Step ahead slightly to check for more voxels
    NextIntersection.t = 0.00001;
  }

  // Keep raymarching
  CurrentT += NextIntersection.t;
  if (CurrentT > EndT) {
    if (ShapeConstant == ELLIPSOID) {
      // For CYLINDER: once clipping / exaggeration is supported, this must be done for cylinders too.
      Intersections = VoxelOctree.ShapeUtils.ListState.GetNextIntersections(IntersectionList);
      if (Intersections.Entry.t == NO_HIT) {
        break;
      } else {
        // Found another intersection. Resume raymarching there
        CurrentT = Intersections.Entry.t;
        EndT = Intersections.Exit.t;
      }
    } else {
        break;
    }

  }

  PositionUV = R.Origin + CurrentT * R.Direction;
  PositionShapeUVSpace = VoxelOctree.ShapeUtils.ConvertUVToShapeUVSpace(PositionUV, JacobianT);
  VoxelOctree.ResumeTraversal(PositionShapeUVSpace, Traversal, Sample);
  NextIntersection = VoxelOctree.GetNextVoxelIntersection(Sample, RawDirection, Intersections, JacobianT, CurrentT);
}

// Convert the alpha from [0,ALPHA_ACCUMULATION_MAX] to [0,1]
AccumulatedResult.a /= ALPHA_ACCUMULATION_MAX;

return AccumulatedResult;
