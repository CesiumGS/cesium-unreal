These tests measure network performance when downloading Google P3DT tiles.

Each one uses a slightly different methodology that could yield better / worse result. 

The test files are a subset of what's downloaded when running the Cesium for Unreal automation test `LocaleChrysler`. 
It becomes a very useful comparison when running that automation test in Unreal. 
It establishes a fastest possible baseline, where we can measure load speed as if there is no other game engine overhead.


Setup you machine
-
1) Install Unreal
2) Install the Cesium for Unreal plugin and samples project
3) Install DB Browser for SQLite
4) Install Python
5) Optionally install Wireshark


Start a Google P3DT session
-
1) Start any app that uses Google P3DT
  * Could load level 12 from the Cesium for Unreal samples
  * Could also run any of the Cesium Google PD3T automation tests
2) Find you sqlite cache at `C:\Users\<your username>\AppData\Local\UnrealEngine\5.4`
3) Open DB Browser for SQLite and open up cesium-request-cache.sqlite
4) You'll see many entries. Note the name `https://tile.googleapis.com/...`
5) In the name you should see the `session=somevalue` embedded
6) Copy this session value to `TestKey.py`

> Note: Sessions don't last forever. You'll have to repeat these steps if running the same tests the next day


Run the network test
-
1) Run any of these
- CurlExeTest.py
- PyRequestLibTest.py
- PyUrlLib3Test.py
- ThreadedRequestsTest.py

2) Note the output and the speed detected. You can optionally start a Wireshark session, then run the test, and view the speed results from Wireshark

> Note: In TestUtils.py, you'll notice a set of 4 files as the test files to download. These as the first 4 files downloaded when running the Cesium for Unreal "LocaleChrysler". These filenames may get out of date. You might also want to download more / less files. Feel free to edit to your needs
