#pragma once

#include <cstdint>

namespace Cesium3DTiles {

    class TileSelectionState {
    public:
        enum class Result {
            /**
             * There was no selection result, perhaps because the tile wasn't visited last frame.
             */
            None = 0,

            /**
             * This tile was deemed not visible and culled.
             */
            Culled = 1,

            /**
             * The tile was selected for rendering.
             */
            Rendered = 2,

            /**
             * This tile did not meet the required screen-space error and was refined.
             */
            Refined = 3,

            /**
             * This tile was originally rendered, but it got kicked out of the render list
             * in favor of an ancestor because it is not yet renderable.
             */
            RenderedAndKicked = 4,

            /**
             * This tile was originally refined, but its rendered descendants got kicked out of the
             * render list in favor of an ancestor because it is not yet renderable.
             */
            RefinedAndKicked = 5
        };

        /**
         * Initializes a new instance with a selection result of `None`.
         */
        TileSelectionState() :
            _frameNumber(0),
            _result(Result::None)
        {}

        /**
         * Initializes a new instance with a given selection result.
         * @param frameNumber The frame number in which the selection took place.
         * @param result The result of the selection.
         */
        TileSelectionState(uint32_t frameNumber, Result result) :
            _frameNumber(frameNumber),
            _result(result)
        {}

        /**
         * Gets the result of selection. The given frame number must
         * match the frame number in which selection last took place.
         * Otherwise, /ref Result::None is returned.
         * @param frameNumber The previous frame number.
         * @return The selection result.
         */
        Result getResult(uint32_t frameNumber) const {
            if (this->_frameNumber != frameNumber) {
                return Result::None;
            }
            return this->_result;
        }

        /**
         * Determines if this tile or its descendents were kicked from the render list.
         * In other words, if its last selection result was /ref Result::RenderedAndKicked
         * or /ref Result::RefinedAndKicked.
         * @param frameNumber The previous frame number.
         * @return true The tile was kicked.
         * @return false The tile was not kicked.
         */
        bool wasKicked(uint32_t frameNumber) const {
            Result result = this->getResult(frameNumber);
            return result == Result::RenderedAndKicked || result == Result::RefinedAndKicked;
        }

        /**
         * Gets the original selection result prior to being kicked.
         * If the tile wasn't kicked, the original value is returned.
         * @param frameNumber The previous frame number.
         * @return The selection result prior to being kicked.
         */
        Result getOriginalResult(uint32_t frameNumber) const {
            Result result = this->getResult(frameNumber);

            switch (result) {
            case Result::RefinedAndKicked:
                return Result::Refined;
            case Result::RenderedAndKicked:
                return Result::Rendered;
            default:
                return result;
            }
        }

        /**
         * Marks this tile as "kicked".
         */
        void kick() {
            switch (this->_result) {
            case Result::Rendered:
                this->_result = Result::RenderedAndKicked;
                break;
            case Result::Refined:
                this->_result = Result::RefinedAndKicked;
                break;
            }
        }

    private:
        uint32_t _frameNumber;
        Result _result;
    };

}
