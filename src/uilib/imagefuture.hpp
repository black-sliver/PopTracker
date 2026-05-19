#pragma once

#include <SDL2/SDL_surface.h>
#include "size.h"

namespace Ui {

class ImageFuture {
protected:
    ImageFuture() = default;

public:
    ImageFuture(const ImageFuture&) = delete;
    ImageFuture(ImageFuture&&) = default;
    virtual ~ImageFuture() = default;

    /// Reads necessary data to calculate the size and returns it, blocks if not available yet.
    virtual Size getSize() = 0;
    /// Generates the surface, blocks if not available yet.
    virtual SDL_Surface* getSurface() = 0;
    /// Returns true if the size is (or can be) calculated without blocking, or failed.
    virtual bool isSizeDone() = 0;
    /// Returns true if the surface is (or can be) generated without blocking, or failed.
    virtual bool isSurfaceDone() = 0;
    /// Tell the processing queue that the result is wanted asap.
    virtual void prioritize() = 0;
    /// Cancel any ongoing processing, may block. Important: cancel in the destructor.
    virtual void cancel() = 0;
};

} // namespace Ui
