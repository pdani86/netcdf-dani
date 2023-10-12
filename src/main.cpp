
#include "netcdf.h"
#include <iostream>
#include <vector>
#include <fstream>
#include <optional>
#include <string>
#include <array>
#include <algorithm>

#include "NcFile.h"
#include "colors.h"
#include "gps.h"


void handle_error(int status) {
    std::cout << "error " << status << std::endl;
}


int print_info(const NcFile& ncFile) {
    printf("NetCDF format version: %s\n", (ncFile.fileFormat() == NC_FORMAT_CLASSIC) ? "Classic" : "Enhanced");
    printf("Number of dimensions: %d\n", (int)ncFile.nDims());

    // Get dimension information
    auto ndims = ncFile.nDims();
    for (int dimid = 0; dimid < ndims; dimid++) {
        auto dimlen = ncFile.dims()[dimid];
        auto dimname = ncFile.dimNames()[dimid];
        std::cout << "Dimension " << dimid << " Name='" << dimname << "', Length=" << std::to_string(dimlen) << "\n";
    }

    std::cout << "Variables:\n";
    auto nvars = ncFile.nVariables();
    for(int varIx = 0; varIx < nvars; ++varIx) {
        auto varInfo = ncFile.getVariableInfo(varIx);
        std::cout << "var[" << varInfo.ix << "]: " << varInfo.name << " - " << varInfo.type << " ";
        bool isFirst = true;
        for(const auto& dim : varInfo.dims) {
            if(!isFirst) std::cout << ",";
            isFirst = false;
            std::cout << dim;
        }
        std::cout << "\n";
    }
    std::cout.flush();

    return 0;
}

std::array<uint8_t, 3> heightToRgb(int64_t height, int16_t max = 9000, int16_t min = -12000) {
    auto range = max - min;
    double t = 0.0;
    double V = 100.0;
    t = height / static_cast<double>(range);
    if(height < 0) {
        V = std::abs(height / (double)min) * 100.0;
    } else {
//        t = (height+min) / static_cast<double>(range);
        t = (height-min) / static_cast<double>(max);
        t = std::clamp(t, 0.0, 1.0);
        V = 100.0 * (0.5 + 0.5 * (height / (double)max));
    }
    auto H = 240.0 + 90.0 * t;
    return HSVtoRGB(H, 100.0f, V);
}

int transform_elevation_data(const NcFile& ncFile) {
    constexpr int scale = 40;

    auto dimlen = ncFile.dims();
    int elevation_var_id = ncFile.getVarIdByName("elevation");

    std::cout << "dimlen: " << dimlen[0] << " * " << dimlen[1] << std::endl;

    size_t width = dimlen[0];
//    size_t height = dimlen[1];

    size_t count[2];
    count[0] = 1;
    count[1] = width;


    std::cout << "Elevation var id: " << elevation_var_id << std::endl;
//    int nRowPerBlockRead = 1000;
    int nRowPerBlockRead = 1;
    auto rowBuf = std::make_unique<int16_t[]>(nRowPerBlockRead * width);

    std::vector<int16_t> rawImageData_16;
    std::vector<uint8_t> rawImageData;
    std::vector<uint8_t> rawImageColorData;
    size_t scaledDownSize[] = {dimlen[0] / scale, dimlen[1] / scale};
    auto rawSize = (scaledDownSize[0]) * (scaledDownSize[1]);
    rawImageData.resize(rawSize);
    rawImageData_16.resize(rawSize);
    rawImageColorData.resize(rawSize * 3);

    for(int rowIx = 0; rowIx < dimlen[1]; rowIx += scale) {
//        if(0 == rowIx % 100) {
//            std::cout << "row " << rowIx << std::endl;
//        }

        size_t start[2] = {(size_t)rowIx, 0};
        ncFile.getInt64Data(rowBuf.get(), elevation_var_id, start, count);

        for(int colIx = 0; colIx < dimlen[0]; colIx += scale) {
            std::int64_t sum = 0;
            sum = rowBuf[colIx];
            for(int i=0;i<scale;++i) {
                sum += rowBuf[colIx + i];
            }
            int avg= ((sum / scale));
            auto ix = (rowIx/scale) * scaledDownSize[0] + (colIx/scale);
            rawImageData_16[ix] = avg;
            rawImageData[ix] = (avg>>8) + 128;
            auto rgb = heightToRgb(avg);
            rawImageColorData[3*ix + 0] = rgb[0];
            rawImageColorData[3*ix + 1] = rgb[1];
            rawImageColorData[3*ix + 2] = rgb[2];
        }
    }
    std::ofstream ofs("out.raw", std::ios::binary);
    ofs.write((const char*)rawImageData.data(), rawImageData.size());
    ofs.flush();

    std::ofstream ofs_color("out_whole_rgb.raw", std::ios::binary);
    ofs_color.write((const char*)rawImageColorData.data(), rawImageColorData.size());
    ofs_color.flush();

    std::ofstream ofs_16("out_16.raw", std::ios::binary);
    ofs_16.write((const char*)rawImageData_16.data(), rawImageData_16.size() * sizeof(int16_t));
    ofs_16.flush();


//    for(int colIx = 0; colIx < width; colIx += 1000) {
//        std::cout << "[" << colIx << "]: " << (int)rowBuf[colIx] << "\n";
//    }
    std::cout << std::endl;

    return 0;
}


int crop_from_elevation_data(const NcFile& ncFile, GpsArea area) {
    const auto w = ncFile.dims().at(0);
    const auto h = ncFile.dims().at(1);
    int elevation_var_id = ncFile.getVarIdByName("elevation");
    const double stepPerDegree = static_cast<double>(w) / 360.0;
    Offset2D center{static_cast<std::size_t>(90.0 * stepPerDegree), static_cast<std::size_t>(180.0 * stepPerDegree)};
//    GpsToOffsetConverter gpsToOffsetConverter(stepPerDegree, center.latLon[0], center.latLon[1]);
    GpsToOffsetConverter gpsToOffsetConverter(stepPerDegree, h/2, w/2);
    auto offsetMin = gpsToOffsetConverter.convert(area.min);
    auto offsetMax = gpsToOffsetConverter.convert(area.max);
//    std::swap(offsetMax.latLon[0], offsetMin.latLon[0]);
    Offset2D areaSize = offsetMax - offsetMin;
//    areaSize.latLon[0] *= 3;
//    areaSize.latLon[1] *= 3;
    std::cout << "area size: " << areaSize.latLon[0] << "*" << areaSize.latLon[1] << std::endl;

    std::vector<int16_t> buf;
    std::vector<uint8_t> buf_u8;
    std::vector<uint8_t> buf_color;
    auto bufSize = areaSize.latLon[0] * areaSize.latLon[1];
    buf.resize(bufSize);
    buf_u8.resize(bufSize);
    buf_color.resize(bufSize * 3);
    std::array<std::size_t, 2> offset{offsetMin.latLon[0], offsetMin.latLon[1]};
    std::array<std::size_t, 2> count{areaSize.latLon[0], areaSize.latLon[1]};
    ncFile.getInt64Data(buf.data(), elevation_var_id, offset.data(), count.data());

    auto minMax16 = std::minmax_element(buf.begin(), buf.end());
    auto min16 = *minMax16.first;
    auto max16 = *minMax16.second;
    auto range = max16 - min16;

    for(size_t ix = 0; ix < bufSize; ++ix) {
        int val = buf[ix];
        auto rgb = heightToRgb(val, 1800, -500);
        val += std::abs(std::numeric_limits<int16_t>::min());
        auto val_u8 = val >> 8;
        buf_u8[ix] = val_u8;
//        auto t = ((buf[ix]-min16)/(double)range);
//        auto H = 240.0 + 90.0 * t;
//        auto rgb = HSVtoRGB(H, 100.0f, 100.0f);
        buf_color[3*ix] = rgb[0];
        buf_color[3*ix+1] = rgb[1];
        buf_color[3*ix+2] = rgb[2];
        buf[ix] = val;
    }
    std::ofstream ofs("out_hun.raw", std::ios::binary);
    std::ofstream ofs_u8("out_hun_u8.raw", std::ios::binary);
    std::ofstream ofs_color("out_hun_rgb.raw", std::ios::binary);
    ofs.write((const char*)buf.data(), buf.size()*sizeof(int16_t));
    ofs_u8.write((const char*)buf_u8.data(), buf_u8.size()*sizeof(uint8_t));
    ofs_color.write((const char*)buf_color.data(), buf_color.size());
    return 0;
}

void transform_to_raw(NcFile& ncFile) {
    auto dimlen = ncFile.dims();
    int elevation_var_id = ncFile.getVarIdByName("elevation");
    std::cout << "dimlen: " << dimlen[0] << " * " << dimlen[1] << std::endl;
    size_t width = dimlen[0];
    size_t count[2];
    count[0] = 1;
    count[1] = width;
    std::cout << "Elevation var id: " << elevation_var_id << std::endl;
    auto rowBuf = std::vector<int16_t>(width);
    size_t sizeDims[] = {dimlen[0], dimlen[1]};
    std::ofstream ofs("D:/out_full_16.raw", std::ios::binary);
    for(int rowIx = 0; rowIx < dimlen[1]; ++rowIx) {
        size_t start[2] = {(size_t)rowIx, 0};
        ncFile.getInt64Data(rowBuf.data(), elevation_var_id, start, count);
        ofs.write((const char*)rowBuf.data(), rowBuf.size()*sizeof(decltype(rowBuf)::value_type));
    }
    ofs.flush();
}

int main() {
    GPS hunGps1{48.64698758285766, 14.999676527732783};
    GPS hunGps2{45.50885749858355, 23.417102803214963};
    auto hunArea = GpsArea::fromPoints(hunGps1, hunGps2);
    //const char* nc_datafile = "D:\\other\\data\\geo\\gebco_2023\\GEBCO_2023.nc";
    const char* nc_datafile = "C:\\dani\\other\\GEBCO_2023.nc";
    auto nc_file = NcFile::openForRead(nc_datafile);
//    print_info(nc_file);
    transform_to_raw(nc_file); return 0;
    transform_elevation_data(nc_file);
    crop_from_elevation_data(nc_file, hunArea);
    return 0;
}
