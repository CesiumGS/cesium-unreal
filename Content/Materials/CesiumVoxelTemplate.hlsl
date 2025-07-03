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
  
  int ShapeConstant;

  float3 MinBounds;
  float3 MaxBounds;

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
  void Initialize(in int InShapeConstant)
  {
    ShapeConstant = InShapeConstant;
    
    if (ShapeConstant == BOX)
    {
      // Default unit box bounds.
      MinBounds = -1;
      MaxBounds = 1;
    }
  }
  
  /**
   * Tests whether the input ray (Unit Space) intersects the box. Outputs the intersections in Unit Space.
   */
  RayIntersections IntersectBox(in Ray R)
  {
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
      Intersection miss = Utils.NewMissedIntersection();
      return Utils.NewRayIntersections(miss, miss);
    }

    // Compute normals
    float3 directions = sign(R.Direction);
    bool3 isLastEntry = bool3(Utils.Equal(entries, float3(entryT, entryT, entryT)));
    result.Entry.Normal = -1.0 * float3(isLastEntry) * directions;
    result.Entry.t = entryT;
   
    bool3 isFirstExit = bool3(Utils.Equal(exits, float3(exitT, exitT, exitT)));
    result.Exit.Normal = float3(isFirstExit) * directions;
    result.Exit.t = exitT;

    return result;
  }
  
  /**
  * Tests whether the input ray (Unit Space) intersects the shape.
  */
  RayIntersections IntersectShape(in Ray R)
  {
    RayIntersections result;
    
    [branch]
    switch (ShapeConstant)
    {
      case BOX:
        result = IntersectBox(R);
        break;
      default:
        return Utils.NewRayIntersections(Utils.NewMissedIntersection(), Utils.NewMissedIntersection());
    }
    
    // Set start to 0.0 when ray is inside the shape.
    result.Entry.t = max(result.Entry.t, 0.0);

    return result;
  }
  
  /**
  * Scales the input UV coordinates from [0, 1] to their values in UV Shape Space.  
  */
  float3 ScaleUVToShapeUVSpace(in float3 UV)
  {
    return UV;
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
  * Converts the input position (vanilla UV Space) to its Shape UV Space relative to the
  * voxel grid geometry. Also outputs the Jacobian transpose for future use.
  */
  float3 ConvertUVToShapeUVSpace(in float3 UVPosition, out float3x3 JacobianT)
  {
    switch (ShapeConstant)
    {
      case BOX:
        return ConvertUVToShapeUVSpaceBox(UVPosition, JacobianT);
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
VoxelOctree.ShapeUtils.Initialize(ShapeConstant);

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

RayIntersections Intersections = VoxelOctree.ShapeUtils.IntersectShape(R);
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

// Account for y-up -> z-up conventions for certain shapes.
switch (ShapeConstant) {
  case BOX:
    DataTextures.GridDimensions = round(GridDimensions.xzy);
    DataTextures.PaddingBefore = round(PaddingBefore.xzy);
    DataTextures.PaddingAfter = round(PaddingAfter.xzy);
    break;
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

float3 RawDirection = R.Direction;

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
    break;
  }

  PositionUV = R.Origin + CurrentT * R.Direction;
  PositionShapeUVSpace = VoxelOctree.ShapeUtils.ConvertUVToShapeUVSpace(PositionUV, JacobianT);
  VoxelOctree.ResumeTraversal(PositionShapeUVSpace, Traversal, Sample);
  NextIntersection = VoxelOctree.GetNextVoxelIntersection(Sample, RawDirection, Intersections, JacobianT, CurrentT);
}

// Convert the alpha from [0,ALPHA_ACCUMULATION_MAX] to [0,1]
AccumulatedResult.a /= ALPHA_ACCUMULATION_MAX;

return AccumulatedResult;
