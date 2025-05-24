#ifndef _UI_MAPWIDGET_H
#define _UI_MAPWIDGET_H

#include "../uilib/image.h"
#include "../core/location.h"
#include "../core/locationsection.h"
#include <map>
#include <vector>

namespace Ui {

class MapWidget final : public Image {
public:
    typedef ::Location::MapLocation::Shape Shape;

    MapWidget(int x, int y, int w, int h, const char* filename);
    MapWidget(int x, int y, int w, int h, const void* data, size_t len);
    
    struct Point {
        int x = 0;
        int y = 0;
        int size = 0;
        int borderThickness = 0;
        Shape shape = Shape::UNSPECIFIED;
        int state = 1;
        Highlight highlight = Highlight::NONE;
    };

    struct Location {
        std::vector<Point> pos;
    };

    // TODO: enum location state
    void addLocation(const std::string& name, Point&& point);
    void setLocationState(const std::string& name, int state, size_t n);
    void setLocationHighlight(const std::string& name, Highlight highlight, size_t n);

    // FIXME: this does not work if name is not unique
    Signal<const std::string&,int,int> onLocationHover; // FIXME: we should provide absolute AND relative mouse position through the Event stack

    void render(Renderer renderer, int offX, int offY) override;
    int getAbsLeft() const { return _absX; } // FIXME: this is not really a good solution
    int getAbsTop() const { return _absY; }

    void setHideClearedLocations(bool hide) { _hideClearedLocations = hide; }
    void setHideUnreachableLocations(bool hide) { _hideUnreachableLocations = hide; }

    static Color StateColors[17];
    static std::map<Highlight, Color> HighlightColors;
    static bool SplitRects;

protected:
    int _absX=0;
    int _absY=0;
    std::map<std::string, Location> _locations;
    std::string _locationHover; // TODO; store iterator instead of string?

    bool _hideClearedLocations = false;
    bool _hideUnreachableLocations = false;

private:
    void connectSignals();
    void calculateSizes(int left, int top, int& srcw, int& srch, int& dstx, int& dsty, int& dstw, int& dsth);
};

} // namespace Ui

#endif
