#pragma once

#include <cstdint>

namespace Cesium3DTiles {

    class TileSelectionState {
    public:
        enum Result {
            /// <summary>
            /// There was no selection result, perhaps because the tile wasn't visited last frame.
            /// </summary>
            None = 0,

            /// <summary>
            /// This tile was deemed not visible and culled.
            /// </summary>
            Culled = 1,

            /// <summary>
            /// The tile was selected for rendering.
            /// </summary>
            Rendered = 2,

            /// <summary>
            /// This tile did not meet the required screen-space error and was refined.
            /// </summary>
            Refined = 3
        };

        TileSelectionState() :
            _frameNumber(-1),
            _result(Result::None)
        {}

        TileSelectionState(int32_t currentFrameNumber, Result result) :
            _frameNumber(currentFrameNumber),
            _result(result)
        {}

        Result getResult(int32_t frameNumber) const {
            if (this->_frameNumber != frameNumber) {
                return Result::None;
            }
            return this->_result;
        }

    private:
        int32_t _frameNumber = -1;
        Result _result;
    };

}
