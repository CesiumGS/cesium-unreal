`Tileset`, and all other objects accessible through it (such as `Tile` instances), may generally only be used from one thread at a time. There are some very limited exceptions.

* `Tile::getState()` may be called at any time and from any thread.
* When a tile is in the `LoadState::ContentLoading` state, a separate load thread may call `getState`, `setState`, and `getContent`. It may also set `_pRendererResources`.

Tileset shut down process, initiated from the game thread:

* For tiles in any state other than `ContentLoading`, just unload it (no other thread is allowed to be accessing it).
* For tiles in the `ContentLoading` state:
  * Move the tile to the "Destroying" state.
  * Cancel requests if possible (e.g. call CancelRequest on the Unreal Engine request object).
  * Wait for the tile to transition out of the `Destroying` state, indicating that `ContentLoading` has completed or been canceled successfully. It _should_ be ok to block the Unreal Engine game thread while waiting on this because the cancelation / status change happens in the HTTP thread.
  * Unload the tile.
