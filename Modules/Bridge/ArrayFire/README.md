### Dependencies

* [ArrayFire](https://arrayfire.com/download-splash/?redirect_to=/download/)
* [OpenCV](http://opencv.org/)

Both of them are available via package manager on Ubuntu. If they are not
available on your package, you can always download and install them from
respective websites.

### Steps to build ArrayFire bridge module

1. Install the dependencies.
2. Ensure `AF_PATH` environment variable points to ArrayFire installation root.
3. Clone `ITK` and `arrayfire-data` repositories.
3. `cd ITK` assuming you cloned ITK into the folder `ITK`.
4. Change the branch to BridgeArrayFire if you are not already on it.
5. `mkdir build && cd build`
6. `cmake .. -DArrayFire_DIR:PATH="$AF_PATH/share/ArrayFire/cmake" -DMODULE_ITKBridgeArrayFire=ON -DMODULE_ITKVideoBridgeOpenCV=ON -DExternalData_OBJECT_STORES=/path-to/arrayfire-data/ITKExternalData`
7. `make -j4`

That's it. ITK should build fine.

#### Note

Ideally, the data files don't need to be stored on our arrayfire-data repository because
when we do `git gerrit-push` they will get uploaded to itk data store locations. Since, we
currently are developing locally, for other developers to share/test/review this code I have
added the data to arrayfire-data repository.
