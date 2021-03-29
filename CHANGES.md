# Change Log

### v1.0.0 - 2021-03-30 - Initial Release

##### Features  :tada:

- High-accuracy, global-scale WGS84 globe for visualization of real-world 3D content
- 3D Tiles runtime engine to stream massive 3D geospatial datasets, such as terrain, imagery, 3D cities, and photogrammetry
  - Streaming from the cloud, a private network, or the local machine.
  - Level-of-detail selection
  - Caching
  - Multithreaded loading
  - Batched 3D Model (B3DM) content, including the B3DM content inside Composite (CMPT) tiles
  - `quantized-mesh` terrain loading and rendering
  - Bing Maps and Tile Map Service (TMS) raster overlays draped on terrain
- Integrated with Cesium ion for instant access to cloud based global 3D content.
- Integrated with Unreal Engine Editor, Actors and Components, Blueprints, Landscaping and Foliage, Sublevels, and Sequencer.
