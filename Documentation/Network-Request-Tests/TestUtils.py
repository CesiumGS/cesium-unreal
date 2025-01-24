
UnrealStyleHeaders = { "Authorization" : "Bearer ",
               "X-Cesium-Client" : "Cesium For Unreal",
               "X-Cesium-Client-Version" : "2.0.0",
               "X-Cesium-Client-Project" : "CesiumForUnrealSamples",
               "X-Cesium-Client-Engine" : "Unreal Engine 5.3.1-28051148+++UE5+Release-5.3",
               "X-Cesium-Client-OS" : "Windows 10 (22H2) [10.0.19045.3570] 10.0.19045.1.256.64bit",
               "User-Agent" : "Mozilla/5.0 (Windows 10 (22H2) [10.0.19045.3570] 10.0.19045.1.256.64bit) Cesium For Unreal/2.0.0 (Project CesiumForUnrealSamples Engine Unreal Engine 5.3.1-28051148+++UE5+Release-5.3)",
               "Content-Length" : "0",
               "Expect" : "" }

def buildUrlArray(sessionString):
    uriArray = [] 
    uriArray.append("https://tile.googleapis.com/v1/3dtiles/datasets/CgA/files/UlRPVEYuYnVsa21ldGFkYXRhLnBsYW5ldG9pZD1lYXJ0aCxidWxrX21ldGFkYXRhX2Vwb2NoPTk2MixwYXRoPTIxNjAsY2FjaGVfdmVyc2lvbj02.json")
    uriArray.append("https://tile.googleapis.com/v1/3dtiles/datasets/CgA/files/UlRPVEYuYnVsa21ldGFkYXRhLnBsYW5ldG9pZD1lYXJ0aCxidWxrX21ldGFkYXRhX2Vwb2NoPTk2MixwYXRoPTIxNDMsY2FjaGVfdmVyc2lvbj02.json")
    uriArray.append("https://tile.googleapis.com/v1/3dtiles/datasets/CgA/files/UlRPVEYuYnVsa21ldGFkYXRhLnBsYW5ldG9pZD1lYXJ0aCxidWxrX21ldGFkYXRhX2Vwb2NoPTk2MixwYXRoPTIxNDIsY2FjaGVfdmVyc2lvbj02.json")
    uriArray.append("https://tile.googleapis.com/v1/3dtiles/datasets/CgA/files/UlRPVEYuYnVsa21ldGFkYXRhLnBsYW5ldG9pZD1lYXJ0aCxidWxrX21ldGFkYXRhX2Vwb2NoPTk2MixwYXRoPTIwNjEsY2FjaGVfdmVyc2lvbj02.json")

    urlArray = [] 
    for uri in uriArray:
        url = uri + "?" + sessionString
        urlArray.append(url)
    return urlArray


def printSummary(secsElapsed, bytesDownloaded):
    msElapsed = secsElapsed * 1000
    print("\nBytes downloaded: " + str(bytesDownloaded))
    print ("Elapsed time: " + "%4f milliseconds" % (msElapsed) + "\n")

    bytesPerSecond = bytesDownloaded / secsElapsed
    megabytesPerSecond = bytesPerSecond / 1024 / 1024
    megabitsPerSecond = megabytesPerSecond * 8

    print("Bytes / second: " + str(bytesPerSecond))
    print("Megabytes / second: " + str(megabytesPerSecond))
    print("Megabits / second: " + str(megabitsPerSecond))
    print("\nDone\n")
