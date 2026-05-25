#pragma once

#include <string>
#include <SDL2/SDL.h>
#include "imagefuture.hpp"
#include "imghelper.h"
#include "../core/assets.h"
#include "../core/fileutil.h"
#include "../core/fs.h"
#include "../core/singleton.hpp"
#include "../core/taskqueue.hpp"
#include "../core/util.h"

namespace Ui {

class AssetImageFuture final : public ImageFuture {
    using TaskQueueType = pop::TaskQueue<SDL_Surface*>;
    using TaskQueue = pop::Singleton<TaskQueueType>;
    static constexpr size_t NO_TASK = TaskQueueType::NO_TASK;

    Size _size = Size::UNDEFINED;
    SDL_Surface* _surface = nullptr;
    const fs::path _path;
    size_t _taskNumber = NO_TASK;
    bool _prioritized = false;

    void await()
    {
        if (_taskNumber != NO_TASK) {
            // try to cancel and process in foreground, otherwise wait for background result
            if (TaskQueue::get().cancel(_taskNumber)) {
                _surface = LoadImage(_path);
            } else {
                _surface = TaskQueue::get().get(_taskNumber);
            }
            _taskNumber = NO_TASK;
            if (_surface && _size == Size::UNDEFINED) {
                _size.width = _surface->w;
                _size.height = _surface->h;
            } else if (!_surface && _size != Size::UNDEFINED) {
                fprintf(stderr, "Could not load image \"%s\"\n", sanitize_print(_path).c_str());
            } else if (_surface && (_size.width != _surface->w || _size.height != _surface->h)) {
                fprintf(stderr, "Actual size differs from expected size for \"%s\"\n", sanitize_print(_path).c_str());
            } else if (!_surface) {
                fprintf(stderr, "Could not load image \"%s\"\n", sanitize_print(_path).c_str());
            }
        }
        if (_size == Size::UNDEFINED)
            _size = {0, 0};
    }

public:
    explicit AssetImageFuture(const std::string& name)
        : _path(asset(name))
    {
        if (_path.empty())
            fprintf(stderr, "Created AssetImageFuture for empty filename\n");
        // TODO: cache stuff
        std::string data;
        readFile(_path, data, getMaxImageHeaderLength());
        _size = getImageSize(data);
        _taskNumber = TaskQueue::get().enqueue([this] {
            return LoadImage(_path);
        });
    }

    AssetImageFuture(AssetImageFuture&&) = default;

    AssetImageFuture(const AssetImageFuture&) = delete;

    ~AssetImageFuture() override
    {
        cancel();
    }

    Size getSize() override
    {
        if (_size == Size::UNDEFINED)
            await(); // wait for decoding and apply size
        return _size;
    }

    SDL_Surface * getSurface() override
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
