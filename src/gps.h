#ifndef NETCDF_DANI_GPS_H
#define NETCDF_DANI_GPS_H

#include <cstdint>
#include <array>

struct GPS {
    std::array<double,2> latLon;

    double lat() const { return latLon[0];}
    double lon() const { return latLon[1];}
    double& lat() { return latLon[0];}
    double& lon() { return latLon[1];}
};

struct Offset2D {
    std::array<size_t,2> latLon{};

    Offset2D operator+(const Offset2D& rhs) const {
        return Offset2D{latLon[0] + rhs.latLon[0], latLon[1] + rhs.latLon[1]};
    }
    Offset2D operator-(const Offset2D& rhs) const {
        return Offset2D{latLon[0] - rhs.latLon[0], latLon[1] - rhs.latLon[1]};
    }
};

struct Size2D {
    std::array<size_t,2> latLon{};
};

struct GpsArea {
    GPS min;
    GPS max;

    static GpsArea fromPoints(GPS p1, GPS p2) {
        GpsArea area;
        area.min.lat() = std::min(p1.lat(), p2.lat());
        area.min.lon() = std::min(p1.lon(), p2.lon());
        area.max.lat() = std::max(p1.lat(), p2.lat());
        area.max.lon() = std::max(p1.lon(), p2.lon());
        return area;
    }

    double width() const { return max.lat() - min.lat(); }
    double height() const { return max.lon() - min.lon(); }
};

class GpsToOffsetConverter {
public:
    GpsToOffsetConverter(double stepPerDegree, std::size_t centerOffsetLat, std::size_t centerOffsetLon)
            : _stepPerDegree(stepPerDegree)
            , _centerOffsetLat(centerOffsetLat)
            , _centerOffsetLon(centerOffsetLon) {}

    Offset2D convert(GPS gps) const {
        return {
                static_cast<std::size_t>(_centerOffsetLat + static_cast<std::int64_t>(gps.lat() * _stepPerDegree)),
                static_cast<std::size_t>(_centerOffsetLon + static_cast<std::int64_t>(gps.lon() * _stepPerDegree))
        };
    }
private:
    double _stepPerDegree{};
    std::int64_t _centerOffsetLat{};
    std::int64_t _centerOffsetLon{};
};

#endif //NETCDF_DANI_GPS_H
