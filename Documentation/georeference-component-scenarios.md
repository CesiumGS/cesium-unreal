
# CesiumGeoreferenceComopnent expected behavior

Explanation of the "authority" column in the table below:
* `Unreal`: The Unreal Location and Rotation properties are the authority, and the ECEF translation and rotation are computed from those properties.
* `ECEF`: The ECEF translation and rotation are the authority, and the Unreal Location and Rotation properties are computed from the ECEF translation and rotation.
* `Both`: Both the ECEF translation and rotation and the Unreal Location and Rotation properties are authoritative, and neither should be computed from the other

| *Operation* | *Authority* | *Sequence* | *Notes* |
|--------------|---------------|---------------|----|
| 1. The CesiumGeoreferenceComponent is added to an existing Actor | `Unreal` | 1. PostInitProperties<br/>2. OnComponentCreated<br/>3. OnRegister | |
| 2. The Unreal location or rotation is changed in the editor | `Unreal` | 1. HandleActorTransformUpdated | |
| 3. The Unreal location or rotation is changed by physics | `Unreal` | 1. HandleActorTransformUpdated | |
| 4. The Unreal location or rotation is changed by Blueprints or by C++ code (e.g. SetWorldTransform) | `Unreal` | 1. HandleActorTransformUpdated | |
| 5. The world OriginLocation is changed (i.e. an origin rebase) | `ECEF` | 1. ApplyWorldOffset<br/>2. HandleActorTransformUpdated (only when recomputing from ECEF actually changes the position) | |
| 6. The Georeference Actor's origin is changed | `ECEF` | 1. HandleGeoreferenceUpdated<br/>2. HandleActorTransformUpdated<br/>3. HandleGeoreferenceUpdated<br/>4. HandleActorTransformUpdated | |
| 7. Any of the Longitude/Latitude/Height/X/Y/Z properties of the component are changed in the Editor | `ECEF` | 1. PreEditChange<br/>2. OnUnregister<br/>3. PostEditChangeChainProperty<br/>4. PostEditChangeProperty<br/>5. OnRegister | |
| 8. Actor with an attached Component is loaded. | `Both` | 1. PostInitProperties<br/>2. PostLoad<br/>3. OnRegister | |
| 9. Actor with an attached Component is pasted. | `Both` | 1. PostInitProperties<br/>2. PreEditChange<br/>3. PostEditChangeProperty<br/>4. PreEditChange<br/>5. PostEditChangeProperty<br/>6. OnComponentCreated<br/>7. OnRegister<br/>8. OnUnregister<br/>9. OnRegister<br/>10. OnUnregister<br/>11. OnRegister | |
| 10. Actor with an attached Component is restored by an undo/repo. | `Both` | 1. PreEditUndo<br/>2. PreEditChange<br/>3. PostEditUndo<br/>4. PostEditChangeProperty<br/>5. OnRegister | |
| 11. An existing CesiumGeoreferenceComponent is copy/pasted onto an existing Actor | `Unreal` | 1. PostInitProperties<br/>2. PreEditChange<br/>3. PostEditChangeProperty<br/>4. PostInitProperties<br/>5. PreEditChange<br/>6. PostEditChangeProperty<br/>7. PostInitProperties<br/>8. PreEditChange<br/>9.PostEditChangeProperty<br/>10. OnComponentCreated<br/>11. OnRegister | This one is arguable (versus the alternative where the Actor jumps to the ECEF location/rotation embodied in the new Component), but I think "don't change the Actor's position just by attaching a component" is a good policy, and the symmetry with (1) is likely to make it easier to implement than the alternative. |
| 12. A CesiumGeoreferenceComponent is deleted from an Actor, and then the user Undoes that operation. | `Both` | 1. PreEditUndo<br/>2. PreEditChange<br/>3. PostEditUndo<br/>4. PostEditChangeProperty<br/>5. OnRegister | Ideally both the Actor Location/Rotation and the ECEF translation/rotation would be preserved in this scenario. This way the Undo would actually restore the exact state rather than an approximation of it. |
| 13. A CesiumGeoreferenceComponent is Cut from an Actor, and then Pasted into the same Actor. | `Unreal` | 1. PostInitProperties<br/>2. PreEditChange<br/>3. PostEditChangeProperty<br/>4. PostInitProperties<br/>5. PreEditChange<br/>6. PostEditChangeProperty<br/>7. PostInitProperties<br/>8. PreEditChange<br/>9. PostEditChangeProperty<br/>10. OnComponentCreated<br/>11. OnRegister | It might be nice to preserve both sets of properties in this scenario when neither has changed in between the Cut and Paste, but that's likely to be difficult. Just treating the Unreal properties as the authority, consistent with (1) and (11) is unlikely to surprise anyone. |
| 14. Begin Play-in-Editor | `Both` | 1. PostInitProperties<br/>2. PostLoad<br/>3. OnRegister<br/>4. HandleGeoreferenceUpdated<br/>5. HandleActorTransformUpdated | Both sets of properties should be preserved. |

Actions:

* `PostLoad`: Mark ECEF valid
* `OnRegister`: Update ECEF from Actor if ECEF is not yet valid
* `HandleActorTransformUpdated`: Update ECEF, and mark it valid.
* `PostEditChangeProperty`: If ECEF properties were changed, mark ECEF valid.
* `PostEditUndo`: Nothing special, because ECEF validity state should be undone as well, as necessary.
* `OnComponentCreated`: ECEF is marked invalid
