#include <cassert>
#include <cstring>
#include <SDL2/SDL.h>
#include <sltbench/Bench.h>
#include "../../src/uilib/colorhelper.h"


//#define BENCH_INDEXED // results are meaningless; enable this if you suspect a regression in palette handling


namespace Ui {} // colorhelper changed namespace, so make sure Ui is always defined


using namespace Ui;


class SurfaceFixture {
public:
    typedef SDL_Surface* Type;

protected:
    Type _surf = nullptr;
};

class RGBASurfaceFixture final : public SurfaceFixture {
public:
    Type& SetUp()
    {
        _surf = SDL_CreateRGBSurfaceWithFormat(0, 256, 256, 32, SDL_PIXELFORMAT_ARGB8888);
        assert(_surf);
        return _surf;
    }

    void TearDown()
    {
        SDL_FreeSurface(_surf);
        _surf = nullptr;
    }
};

class IndexedSurfaceFixture final : public SurfaceFixture {
public:
    Type& SetUp()
    {
        _surf = SDL_CreateRGBSurfaceWithFormat(0, 256, 256, 8, SDL_PIXELFORMAT_INDEX8);
        assert(_surf && _surf->format && _surf->format->palette);
        SDL_Color colors[256];
        for (int i = 0; i < 256; i++) {
            uint32_t val = static_cast<uint32_t>(random());
            memcpy(&colors[i], &val, sizeof(colors[i]));
        }
        SDL_SetPaletteColors(_surf->format->palette, colors, 0, 256);
        return _surf;
    }

    void TearDown()
    {
        SDL_FreeSurface(_surf);
        _surf = nullptr;
    }
};

/// Benchmarks the default "@disabled" filter for a given surface
void MakeDisabled(SDL_Surface*& surf)
{
    makeGreyscale(surf, true);
}

#if defined(UI_COLORHELPER_HAS_SATURATION) || defined(UI_COLORHELPER_HAS_BRIGHTNESS)
volatile float _half = 0.5f;
#endif

#ifdef UI_COLORHELPER_HAS_SATURATION
/// Benchmarks the "greyscale|bt601" filter for a given surface
void GreyscaleLuminosity(SDL_Surface*& surf)
{
    setSaturation<BWMode::Luminosity>(surf, 0.0f);
}

/// Benchmarks the "greyscale|non-bt601" filter for a given surface
void GreyscaleAverage(SDL_Surface*& surf)
{
    setSaturation<BWMode::Average>(surf, 0.0f);
}

/// Benchmarks the "saturation|<f>|bt601" filter for a given surface
void HalfSaturationLuminosity(SDL_Surface*& surf)
{
    const float value = _half;
    setSaturation<BWMode::Luminosity>(surf, value);
}

/// Benchmarks the "saturation|<f>|non-bt601" filter for a given surface
void HalfSaturationAverage(SDL_Surface*& surf)
{
    const float value = _half;
    setSaturation<BWMode::Average>(surf, value);
}
#endif

#ifdef UI_COLORHELPER_HAS_BRIGHTNESS
/// Benchmarks the "dim" filter for a given surface
void Dim(SDL_Surface*& surf)
{
    setBrightness(surf, 0.5f);
}

/// Benchmarks the "brightness|<f>" filter for a given surface
void HalfBrightness(SDL_Surface*& surf)
{
    const float value = _half;
    setBrightness(surf, value);
}
#endif


SLTBENCH_FUNCTION_WITH_FIXTURE(MakeDisabled, RGBASurfaceFixture);
#ifdef BENCH_INDEXED
SLTBENCH_FUNCTION_WITH_FIXTURE(MakeDisabled, IndexedSurfaceFixture);
#endif

#ifdef UI_COLORHELPER_HAS_SATURATION
SLTBENCH_FUNCTION_WITH_FIXTURE(GreyscaleLuminosity, RGBASurfaceFixture);
SLTBENCH_FUNCTION_WITH_FIXTURE(GreyscaleAverage, RGBASurfaceFixture);
SLTBENCH_FUNCTION_WITH_FIXTURE(HalfSaturationLuminosity, RGBASurfaceFixture);
SLTBENCH_FUNCTION_WITH_FIXTURE(HalfSaturationAverage, RGBASurfaceFixture);
#ifdef BENCH_INDEXED
SLTBENCH_FUNCTION_WITH_FIXTURE(GreyscaleLuminosity, IndexedSurfaceFixture);
SLTBENCH_FUNCTION_WITH_FIXTURE(GreyscaleAverage, IndexedSurfaceFixture);
SLTBENCH_FUNCTION_WITH_FIXTURE(HalfSaturationLuminosity, IndexedSurfaceFixture);
SLTBENCH_FUNCTION_WITH_FIXTURE(HalfSaturationAverage, IndexedSurfaceFixture);
#endif
#endif

#ifdef UI_COLORHELPER_HAS_BRIGHTNESS
SLTBENCH_FUNCTION_WITH_FIXTURE(Dim, RGBASurfaceFixture);
SLTBENCH_FUNCTION_WITH_FIXTURE(HalfBrightness, RGBASurfaceFixture);
#ifdef BENCH_INDEXED
SLTBENCH_FUNCTION_WITH_FIXTURE(Dim, IndexedSurfaceFixture);
SLTBENCH_FUNCTION_WITH_FIXTURE(HalfBrightness, IndexedSurfaceFixture);
#endif
#endif
