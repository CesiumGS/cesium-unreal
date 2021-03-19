Cesium for Unreal brings a high-fidelity model of Planet Earth into Unreal Engine. But Earth is huge, bigger than the biggest game worlds, and we can't fake it. For most real-world applications, it's essential that Philadelphia look like Philadelphia, not a generic city filled with procedurally-generated buildings.

[screenshot of a city, preferably the one mentioned in the paragraph above]

Representing our massive and intricate planet in Unreal Engine requires two things:

1. specialized techniques for _preserving precision_ in the position of things in the world, and
2. a _lot_ of data.

Techniques for preserving precision are necessary because Earth coordinate values are very large. In a standard Cartesian coordinate system centered at the center of the Earth, positions on the Earth's surface have a magnitude of over 6.3 million meters. The standard numeric representation used in game engines - the 32-bit, single-precision floating point number - can only accurately represent a position of this magnitude down to about a meter, at best. Without the specialized techniques used by Cesium for Unreal, this imprecision means that objects would appear to jump around on the screen or "jitter" due to floating-point rounding errors as an object or the camera move.

[jitter video?]

When we say that representing Earth requires a lot of data, we are talking on the order of petabytes (millions of gigabytes). This is far more data than can be realistically stored on a single end-user system. By tapping into Cesium ion and the entire Cesium platform, Cesium for Unreal can stream massive real-world data from the Cloud into an Unreal Engine application as it is needed.  We can view the Earth from afar by streaming in a low-detail representation of the entire planet. Or we can zoom in close to a detailed model of a building and examine the grout lines between individual bricks.

Cesium for Unreal allows us to use the entire world as context for our applications, and in fact allows our users to roam anywhere on the planet. At the same time, it allows us to use the normal toolbox of Unreal Actors, Pawns, Components, and more to add sophisticated visualization, interactivity, and realistic physics anyhere on the globe. Achieving both of these things together requires us to exercise some care in how we build our levels. The rest of this guide covers best-practices for building worlds with Cesium for Unreal.

# A single area of interest, with global context

The simplest world we can build with Cesium for Unreal has a single primary area of interest, maybe the size of a city. Within that area, we want to use all of the features of Unreal Engine, such as:

* Allow players to control a first-person character that can run and jump through the city, obeying normal Unreal Engine physics.
* Allow players to get in a car and drive it around the city.
* Use DataSmith to import a CAD model of a planned building.
* Use the Foliage Tool to place trees and other plants around the area.
* Record videos.

But we want that city to be accurately located in the world, and we want to allow the user to visit elsewhere on the globe if they like. Other places on the globe won't have any custom Unreal Engine objects, but they _will_ have accurate terrain, imagery, and Cesium 3D Tiles models. This is easy to achieve with Cesium for Unreal.

First, create a new Level and add "Cesium World Terrain + Bing Maps Aerial imagery" to it using the Cesium panel:

[picture]

As soon as we add these assets, we'll see that the world around us has changed so that we're in the mountains West of Denver, looking toward the city. Why Denver? Has the Unreal Editor camera been moved? Not at all! When we added the `Cesium3DTileset` to the level, a `CesiumGeoreference` Actor was automatically added alongside it. This Actor controls where the Unreal Engine world is located on the globe. By default, the `CesiumGeoreference` has an "Origin Longitude", "Origin Latitude", and "Origin Height" corresponding to this position in the mountains West of Denver.

[Picture of CesiumGeoreference properties]

Whatever Origin location is specified in the `CesiumGeoreference` becomes the origin (0,0,0) of the regular Unreal Engine world. Furthermore, the world is oriented so that Unreal's +X axis points East at that point, its +Y axis points South, and its +Z axis points up along the ellipsoid surface normal. This is extremely convenient because it gives us a fairly normal Unreal coordinate system centered at a point on the globe of our choosing. Drop a car Actor into the scene, and it will be upright. Gravity, which pulls in the -Z direction in Unreal's physics engine, will pull objects toward the ground. We can use all of the normal Unreal Engine tools at our disposal to design an amazing level in and around Denver, and everything just works.

By changing the Origin position, we can place our level in another city, while all of the objects in the level remain fixed relative to each other and to the viewer.

And yet, even though our level is centered in a particular location, we can increase the speed of the camera and fly off to another city, even on the other side of the world, and get a high-fidelity visualization there as well. By default, the `CesiumGeoreference` also has another important job: it periodically moves the world's `OriginLocation` to keep it near the viewer. TODO: should it?

While our `Cesium3DTileset` works great anywhere in the world, other Unreal Engine objects start to misbehave as we get farther from Denver. Because the Earth is round, gravity is not in the -Z direction everywhere; its direction changes depending on our location on the globe. 

TODO: talk about GlobeAwareDefaultPawn here?

# Multiple areas of interest, all with global context

# Fully globe-aware objects
