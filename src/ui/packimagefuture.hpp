#pragma once

#include <string>
#include <utility>
#include <SDL2/SDL.h>
#include "../core/pack.h"
#include "../core/singleton.hpp"
#include "../core/taskqueue.hpp"
#include "../core/util.h"
#include "../uilib/imagefuture.hpp"

namespace Ui {

// Ownership model:
// Future is owned by a child of the root view
// Pack is owned by Tracker
// Root view is co-owned by Tracker, because it's invalid if Tracker is gone
// Teardown: unload Tracker -> destroy root view: cancel futures, destroy Pack

class PackImageFuture final : public ImageFuture {
    using TaskQueueType = pop::TaskQueue<SDL_Surface*>;
    using TaskQueue = pop::Singleton<TaskQueueType>;
    static constexpr size_t NO_TASK = TaskQueueType::NO_TASK;

    Size _size = Size::UNDEFINED;
    SDL_Surface* _surface = nullptr;
    const Pack* const _pack;
    const std::string _filename;
    size_t _taskNumber = NO_TASK;
    bool _prioritized = false;

    void await()
    {
        if (_taskNumber != NO_TASK) {
            // try to cancel and process in foreground, otherwise wait for background result
            if (TaskQueue::get().cancel(_taskNumber)) {
                _surface = _pack->getImage(_filename);
            } else {
                _surface = TaskQueue::get().get(_taskNumber);
            }
            _taskNumber = NO_TASK;
            if (_surface && _size == Size::UNDEFINED) {
                _size.width = _surface->w;
                _size.height = _surface->h;
            } else if (!_surface && _size != Size::UNDEFINED) {
                fprintf(stderr, "Could not load image \"%s\"\n", sanitize_print(_filename).c_str());
            } else if (_surface && (_size.width != _surface->w || _size.height != _surface->h)) {
                fprintf(stderr, "Actual size differs from expected size for \"%s\"\n", sanitize_print(_filename).c_str());
            } else if (!_surface) {
                fprintf(stderr, "Could not load image \"%s\"\n", sanitize_print(_filename).c_str());
            }
        }
        if (_size == Size::UNDEFINED)
            _size = {0, 0};
    }

public:
    PackImageFuture(const Pack* pack, std::string filename)
        : ImageFuture()
        , _pack(pack)
        , _filename(std::move(filename))
    {
        if (_filename.empty())
            fprintf(stderr, "Created PackImageFuture for empty filename\n");
        _size = _pack->getImageSize(_filename);
        _taskNumber = TaskQueue::get().enqueue([this] {
            return _pack->getImage(_filename);
        });
    }

    PackImageFuture(PackImageFuture&&) = default;

    PackImageFuture(const PackImageFuture&) = delete;

    ~PackImageFuture() override
    {
        cancel();
    }

    Size getSize() override
    {
        if (_size == Size::UNDEFINED)
            await(); // wait for decoding and apply size
        return _size;
    }

    SDL_Surface* getSurface() override
    {
        await();
        SDL_Surface* surface = _surface;
        _surface = nullptr; // can only get it once because of ownership
        return surface;
    }

    bool isSizeDone() override
    {
        return true;
    }

    bool isSurfaceDone() override
    {
        return _taskNumber == NO_TASK || TaskQueue::get().isDone(_taskNumber);
    }

    void prioritize() override
    {
        if (_prioritized)
            return;
        if (_taskNumber == NO_TASK)
            return;
        TaskQueue::get().prioritize(_taskNumber);
        _prioritized = true;
    }

    void cancel() override
    {
        if (_taskNumber != NO_TASK) {
            if (TaskQueue::get().cancel(_taskNumber))
                _taskNumber = NO_TASK;
            else
                await();
        }
        SDL_Surface* surface = _surface;
        _surface = nullptr;
        SDL_FreeSurface(surface);
    }
};

} // namespace Ui
