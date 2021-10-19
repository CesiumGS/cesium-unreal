It can be a challenge to develop a UObject-derived class that can be used from C++, from Blueprints, and in the Editor while ensuring internal states stay consistent. Here are some tips:

# UPROPERTY

* Add `VisibleAnywhere` or `EditAnywhere` depending on whether the property is read-only or editable in the Details panel in the Editor. Or omit both of these if it should not show up in the Editor at all.
* Add `BlueprintReadOnly` or `BlueprintReadWrite` depending on whether the property is read-only or editable from Blueprints. Or omit both of these if it should now be accessible from Blueprints at all.
* Override the `PostEditChangeProperty` method to be notified of changes to UPROPERTY values in the Editor.
* Add a `BlueprintSetter` attribute to each property to be notified of changes to UPROPERTY values from Blueprints.
* In almost all cases, UPROPERTY declarations should appear in the `private:` section of the class and have `Meta = (AllowPrivateAccess)`. This prevents them from being get/set directly from C++ code outside the class, which is important because there is no mechanism like `PostEditChangeProperty` or `BlueprintSetter` available from C++ code.
* In the rare case that no action is necessary when setting a property from C++ (and it's unlikely to be needed in the future), the property can be in the `public:` section. Such a property probably doesn't need a `PostEditChangeProperty` or `BlueprintSetter` either.
* The function given to `BlueprintSetter` must be a UFUNCTION. This function should usually be public and it should be the mechanism for setting the property from C++, too. A corresponding `BlueprintGetter` is usually needed for use by C++ even though it's often not needed for Blueprints.
* A UPROPERTY can have type `double`, but such a UPROPERTY cannot be declared Blueprint accessible. It can be useful to declare GET/SET UFUNCTIONs to access these properties as floats. Such functions should be prefixed with "Inaccurate".
